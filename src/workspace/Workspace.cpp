#include "workspace/Workspace.h"
#include "codegen/InterfaceEmitter.h"
#include "codegen/RecordEmitter.h"
#include "codegen/SettingsEmitter.h"
#include "parser/SpecParser.h"
#include "model/TypeGraphBuilder.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

namespace cc::codegen {
namespace {
namespace fs = std::filesystem;
using Json = nlohmann::json;

std::string read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read " + path.string());
    std::ostringstream result;
    result << input.rdbuf();
    if (input.bad() || result.bad()) throw std::runtime_error("read failed: " + path.string());
    return result.str();
}

Json object(const fs::path& path) {
    try {
        auto json = Json::parse(read(path));
        if (!json.is_object()) throw std::runtime_error("expected a JSON object");
        return json;
    } catch (const std::exception& e) {
        throw std::runtime_error(path.string() + ": " + e.what());
    }
}

bool identifier(const std::string& name) {
    if (name.empty() || std::isdigit(static_cast<unsigned char>(name.front()))) return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char ch) { return ch == '_' || std::isalnum(ch); });
}

fs::path outputPath(const Json& value) {
    fs::path path(value.get<std::string>());
    if (path.empty() || path.has_root_path()) throw std::runtime_error("expected relative output path: " + path.string());
    for (const auto& part : path) {
        if (part == "..") throw std::runtime_error("output path escapes its directory: " + path.string());
    }
    return path.lexically_normal();
}

TypeTable loadTypes(const Json& json) {
    TypeTable types;
    if (!json.is_object()) throw std::runtime_error("expected external type mappings");
    for (const auto& [name, value] : json.items()) {
        ExternalType type{value.value("header", std::string{})};
        type.builtin = value.value("builtin", false);
        if (value.contains("ts") || value.contains("tsModule"))
            throw std::runtime_error(name + ": move TypeScript mappings to a .spec binding typescript block");
        types.emplace(name, std::move(type));
    }
    return types;
}

struct SettingsOutput {
    fs::path typescript;
    SettingsBinding binding;
};

std::string bindingName(const std::string& qualified) {
    const auto pos = qualified.rfind("::");
    return pos == std::string::npos ? qualified : qualified.substr(pos + 2);
}

[[noreturn]] void invalidBinding(const TypeScriptBinding& rule, std::string_view backend) {
    throw std::runtime_error(rule.source.location.describe() + ": unsupported " + std::string(backend) + " binding for " + rule.source.name);
}

SettingsBinding settingsBinding(const Module& module) {
    SettingsBinding result;
    for (const auto& rule : module.bindings.typescript) {
        if (rule.kind == BindingTarget::type) continue;
        const auto name = bindingName(rule.source.name);
        if (rule.kind == BindingTarget::constEnum || rule.kind == BindingTarget::classType || rule.names || !rule.methods.empty())
            invalidBinding(rule, "settings");
        if (rule.kind == BindingTarget::interfaceType && !rule.helpers) result.interfaceOnly.insert(name);
        if (rule.registerEnum) result.registerEnums.insert(name);
        if (!rule.renamedFrom.empty()) result.renamedFrom.emplace(name, rule.renamedFrom);
    }
    return result;
}

fs::path typescriptOutput(const Json& config) {
    auto ts = outputPath(config.at("typescript"));
    if (ts.extension() != ".ts") throw std::runtime_error("expected .ts output");
    return ts;
}

RecordOutput loadRecordOutput(const Json& config, const Module& module) {
    RecordOutput output{.header = outputPath(config.at("header"))};
    if (config.contains("nativeRoot")) output.nativeRoot = outputPath(config.at("nativeRoot"));
    if (output.header.extension() != ".h") throw std::runtime_error("expected .h output");
    if (config.contains("forwardHeader")) {
        output.forwardHeader = outputPath(config.at("forwardHeader"));
        if (output.forwardHeader->extension() != ".h" || *output.forwardHeader == output.header)
            throw std::runtime_error("expected a distinct .h forwardHeader");
    }
    if (config.contains("namesHeader")) {
        output.namesHeader = outputPath(config.at("namesHeader"));
        if (output.namesHeader->extension() != ".h" || *output.namesHeader == output.header || output.namesHeader == output.forwardHeader)
            throw std::runtime_error("expected a distinct .h namesHeader");
    }
    if (config.contains("serializationHeader")) {
        const auto path = outputPath(config.at("serializationHeader"));
        if (path.extension() != ".h" || path == output.header || path == output.namesHeader || path == output.forwardHeader)
            throw std::runtime_error("expected a distinct .h serializationHeader");
        output.serializationHeader = path;
    }
    if (config.contains("jsbHeader")) {
        const auto path = outputPath(config.at("jsbHeader"));
        if (path.extension() != ".h" || path == output.header || path == output.namesHeader ||
            path == output.forwardHeader || path == output.serializationHeader)
            throw std::runtime_error("expected a distinct .h jsbHeader");
        output.jsbHeader = path;
    }
    if (config.contains("typescript")) output.typescript = typescriptOutput(config);
    const auto& bindings = module.bindings;
    if (output.jsbHeader.has_value() != bindings.jsb.has_value())
        throw std::runtime_error(module.name + ": configure jsbHeader and binding jsb together");
    output.jsbIncludes = bindings.jsbIncludes;
    for (const auto& [source, location] : bindings.jsbRecords) output.jsbRecords.insert(bindingName(source));
    output.poolModule = bindings.poolModule;
    std::set<std::string> poolMembers;
    for (const auto& rule : bindings.typescript) {
        if (rule.kind == BindingTarget::type) continue;
        const auto name = bindingName(rule.source.name);
        if (!output.typescript || rule.kind == BindingTarget::interfaceType || rule.registerEnum || !rule.renamedFrom.empty())
            invalidBinding(rule, "records");
        if (rule.names) output.namedEnums.insert(name);
        if (rule.kind == BindingTarget::constEnum) output.constEnums.insert(name);
        if (rule.reset == ResetBinding::defaults) output.resetToDefaults.insert(name);
        if (rule.reset == ResetBinding::constructor) output.resetFromConstructor.insert(name);
        if (rule.pool) {
            if (output.poolModule.empty()) throw std::runtime_error(rule.source.location.describe() + ": pool binding requires poolModule");
            if (!poolMembers.insert(rule.pool->member).second)
                throw std::runtime_error(rule.source.location.describe() + ": duplicate pool member " + rule.pool->member);
            output.pools.emplace(name, *rule.pool);
        }
    }
    return output;
}

std::string normalizeLines(const std::string& input, bool crlf) {
    std::string result;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\r' && i + 1 < input.size() && input[i + 1] == '\n') continue;
        if (input[i] == '\n' && crlf) result += '\r';
        result += input[i];
    }
    return result;
}

Files plan(const Json& settings, const fs::path& base) {
    auto types = loadTypes(object(base / settings.at("types").get<std::string>()));
    auto preamble = normalizeLines(read(base / settings.at("preamble").get<std::string>()), false);
    const auto eol = settings.at("lineEnding").get<std::string>();
    if (eol != "crlf" && eol != "lf") throw std::runtime_error("lineEnding must be lf or crlf");
    const auto& modules = settings.at("modules");
    if (!modules.is_array() || modules.empty()) throw std::runtime_error("expected modules array");
    std::vector<ModuleInput> inputs;
    std::vector<std::variant<std::monostate, OutputLayout, SettingsOutput, RecordOutput>> layouts;
    for (const auto& config : modules) {
        const auto spec = base / config.at("spec").get<std::string>();
        auto module = parseModule(spec, read(spec));
        const auto backend = config.value("backend", std::string{"interfaces"});
        if (config.contains("projection"))
            throw std::runtime_error(spec.string() + ": move JSON projection rules into .spec binding blocks");
        if (backend != "records" && (module.bindings.jsb || !module.bindings.poolModule.empty()))
            throw std::runtime_error(spec.string() + ": JSB and poolModule bindings require the records backend");
        if (backend == "model" || backend == "interfaces") {
            for (const auto& rule : module.bindings.typescript)
                if (rule.kind != BindingTarget::type && !(backend == "interfaces" && rule.kind == BindingTarget::interfaceType))
                    invalidBinding(rule, backend);
        }
        if (backend == "model") {
            layouts.emplace_back(std::monostate{});
            inputs.push_back({std::move(module), loadTypes(config.value("types", Json::object())), true});
            continue;
        }
        if (backend == "records") {
            auto output = loadRecordOutput(config, module);
            auto tsModule = output.typescript.value_or(fs::path{});
            tsModule.replace_extension();
            TypeTable mappings;
            for (const auto& decl : module.declarations) {
                if (std::holds_alternative<Enum>(decl.data) || std::holds_alternative<Record>(decl.data) ||
                    std::holds_alternative<GraphDeclaration>(decl.data)) {
                    mappings.emplace(decl.name, ExternalType{.header = output.header.generic_string(),
                        .typescript = decl.name, .typescriptModule = tsModule.generic_string()});
                }
            }
            layouts.emplace_back(std::move(output));
            inputs.push_back({std::move(module), std::move(mappings)});
            continue;
        }
        const auto ts = typescriptOutput(config);
        if (backend == "settings") {
            layouts.emplace_back(SettingsOutput{ts, settingsBinding(module)});
            inputs.push_back({std::move(module), {}});
        } else if (backend == "interfaces") {
            OutputLayout layout{outputPath(config.at("nativeRoot")), outputPath(config.at("includeDir")),
                                config.at("prefix").get<std::string>(), ts};
            if (!identifier(layout.prefix)) throw std::runtime_error("invalid file prefix " + layout.prefix);
            layouts.emplace_back(std::move(layout));
            inputs.push_back({std::move(module), {}});
        } else {
            throw std::runtime_error("unknown backend " + backend);
        }
    }
    const auto graph = buildTypeGraph(inputs, types);
    std::set<std::string> outputKeys;
    Files files;
    const auto addFile = [&](const fs::path& path, const std::string& content) {
        auto relative = path.lexically_normal();
        auto key = relative.generic_string();
        std::ranges::transform(key, key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!outputKeys.insert(key).second) throw std::runtime_error("duplicate output " + key);
        files.emplace(relative, normalizeLines(content, eol == "crlf"));
    };
    for (size_t i = 0; i < layouts.size(); ++i) {
        if (std::holds_alternative<std::monostate>(layouts[i])) continue;
        const ModuleId owner{static_cast<uint32_t>(i)};
        Files emitted;
        if (const auto* settingsOutput = std::get_if<SettingsOutput>(&layouts[i])) {
            emitted.emplace(settingsOutput->typescript, emitSettings(graph, owner, settingsOutput->binding, settingsOutput->typescript, preamble));
        } else if (const auto* records = std::get_if<RecordOutput>(&layouts[i])) {
            std::optional<ArchiveOutput> archive;
            if (records->serializationHeader) {
                const auto& module = graph.modules.at(i);
                for (auto imported : module.imports) {
                    const auto& dependency = graph.modules.at(imported.value);
                    if (dependency.name == "ArchiveInterface") {
                        if (const auto* layout = std::get_if<OutputLayout>(&layouts.at(imported.value)))
                            archive.emplace(*layout, dependency);
                    }
                }
                if (!archive) throw std::runtime_error(module.name + ": serialization requires import ArchiveInterface with interface output");
            }
            emitted = emitRecords(graph, owner, *records, archive, preamble);
        } else {
            emitted = emitInterfaces(graph, owner, std::get<OutputLayout>(layouts[i]), preamble);
        }
        for (const auto& [path, content] : emitted) addFile(path, content);
        if (modules[i].contains("runtimeFiles")) {
            const auto& runtime = modules[i].at("runtimeFiles");
            if (!runtime.is_object()) throw std::runtime_error("expected runtimeFiles output-to-source mapping");
            for (const auto& [destination, source] : runtime.items())
                addFile(outputPath(destination), read(base / source.get<std::string>()));
        }
    }
    return files;
}

// Create each replacement on the destination filesystem, then publish it by rename.
void replaceFile(const fs::path& target, const std::string& bytes) {
    fs::create_directories(target.parent_path());
    std::random_device random;
    fs::path scratch;
    do {
        scratch = target.parent_path() / (".cocos-tmp-" + std::to_string(random()));
    } while (!fs::create_directory(scratch));
    const fs::path temp = scratch / "content";
    try {
        std::ofstream out;
        out.exceptions(std::ios::badbit | std::ios::failbit);
        out.open(temp, std::ios::binary);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        out.close();
        fs::rename(temp, target);
    } catch (...) {
        std::error_code ec;
        fs::remove(temp, ec);
        fs::remove(scratch, ec);
        throw;
    }
    fs::remove(scratch);
}

void printDifference(const std::string& expected, const std::string& actual) {
    const size_t offset = static_cast<size_t>(std::mismatch(expected.begin(), expected.end(),
                                                          actual.begin(), actual.end()).first - expected.begin());
    const auto begin = expected.begin() + static_cast<std::ptrdiff_t>(offset);
    std::cerr << "  first mismatch at byte " << offset << ", line " << 1 + std::count(expected.begin(), begin, '\n')
              << " (expected " << expected.size() << " bytes, found " << actual.size() << ")\n";
    const auto newline = offset ? expected.rfind('\n', offset - 1) : std::string::npos;
    const auto start = newline == std::string::npos ? 0 : newline + 1;
    for (const auto* content : {&actual, &expected}) {
        if (start >= content->size()) continue;
        const auto end = content->find_first_of("\r\n", start);
        std::cerr << (content == &actual ? "- " : "+ ") << content->substr(start, end - start) << '\n';
    }
}

} // namespace

int generate(const fs::path& workspace, const std::optional<fs::path>& output, bool check, bool diff) {
    auto config = object(workspace);
    const auto base = fs::absolute(workspace).parent_path();
    const auto destination = output.value_or(base / config.at("outputRoot").get<std::string>());
    auto root = fs::weakly_canonical(fs::absolute(destination));
    const Files files = plan(config, base);
    bool drift = false;
    for (const auto& [path, bytes] : files) {
        const auto target = root / path;
        if (fs::weakly_canonical(target) != target || fs::is_symlink(target)) {
            throw std::runtime_error("redirected output path: " + target.string());
        }
        const bool exists = fs::exists(target);
        const auto actual = exists ? read(target) : std::string{};
        if (exists && actual == bytes) continue;
        if (check) {
            std::cerr << (exists ? "changed: " : "missing: ") << path.generic_string() << '\n';
            if (diff && exists) printDifference(bytes, actual);
            drift = true;
        } else {
            replaceFile(target, bytes);
        }
    }
    if (check && fs::exists(root)) {
        for (const auto& entry : fs::recursive_directory_iterator(root)) {
            if (!entry.is_regular_file()) continue;
            const auto relative = entry.path().lexically_relative(root);
            if (!files.contains(relative)) {
                std::cerr << "unexpected: " << relative.generic_string() << '\n';
                drift = true;
            }
        }
    }
    return drift ? 1 : 0;
}

} // namespace cc::codegen
