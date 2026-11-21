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

struct ModuleOutput {
    RecordOutput types;
    std::optional<fs::path> settingsFile;
};

std::optional<ArchiveOutput> resolveArchive(const TypeGraph& graph, ModuleId owner, const std::vector<ModuleOutput>& outputs) {
    const auto& module = graph.modules.at(owner.value);
    const auto& output = outputs.at(owner.value).types;
    const auto emitsTypeScriptArchive = [&](TypeId id) {
        const auto& node = graph.node(id);
        if (!node.typescriptName) return false;
        if (const auto* record = std::get_if<RecordType>(&node.data)) return record->serializable;
        if (const auto* g = std::get_if<GraphType>(&node.data)) return g->serializable;
        return false;
    };
    if (!output.serializationHeader && !(output.typescript && std::ranges::any_of(module.declarations, emitsTypeScriptArchive)))
        return std::nullopt;

    std::optional<ArchiveOutput> archive;
    for (auto imported : module.imports) {
        const auto& provider = graph.modules.at(imported.value);
        if (provider.inputArchive == TypeId{}) continue;
        if (archive)
            throw std::runtime_error(module.name + ": ambiguous archive providers " + archive->module.name + " and " + provider.name);
        archive.emplace(outputs.at(imported.value).types, provider);
    }
    if (!archive)
        throw std::runtime_error(module.name + ": serialization requires a direct import providing @archive(input) and @archive(output)");
    if (output.serializationHeader && archive->output.header.empty())
        throw std::runtime_error(module.name + ": serialization requires C++ output from " + archive->module.name);
    return archive;
}

void checkKeys(const Json& object, std::initializer_list<std::string_view> keys, std::string_view context) {
    if (!object.is_object()) throw std::runtime_error(std::string(context) + ": expected an object");
    for (const auto& [key, value] : object.items()) {
        if (std::ranges::find(keys, key) == keys.end())
            throw std::runtime_error(std::string(context) + ": unknown option " + key);
    }
}

// Translate existing workspaces before resolving file names and registering module imports.
Json moduleConfig(Json config, bool external) {
    if (config.contains("projection"))
        throw std::runtime_error("move JSON projection rules into .spec binding blocks");
    if (config.contains("backend") || config.contains("includeDir") || config.contains("header")) {
        if (config.contains("cpp")) throw std::runtime_error("cannot mix cpp and legacy output options");
        const auto backend = config.value("backend", std::string{"interfaces"});
        config.erase("backend");
        Json cpp = Json::object();
        if (backend == "interfaces" || backend == "records") {
            config.erase("emitEmptySource"); // A C++ output always includes its source file.
            if (backend == "interfaces") {
                cpp["prefix"] = config.at("prefix");
                cpp["dir"] = config.at("includeDir");
                config.erase("prefix");
                config.erase("includeDir");
            } else {
                cpp["header"] = config.at("header");
                config.erase("header");
            }
            for (const auto& [oldKey, key] : {std::pair{"nativeRoot", "root"}, {"forwardHeader", "forward"},
                     {"namesHeader", "names"}, {"serializationHeader", "serialization"}, {"jsbHeader", "jsb"},
                     {"forwardIncludes", "forwardIncludes"}}) {
                if (config.contains(oldKey)) {
                    cpp[key] = config.at(oldKey);
                    config.erase(oldKey);
                }
            }
            // Legacy paths are relative to the output root, independent of workspace defaults.
            if (!cpp.contains("root")) cpp["root"] = ".";
            if (!cpp.contains("dir")) cpp["dir"] = ".";
            config["cpp"] = std::move(cpp);
        } else if (backend == "settings") {
            config["typescript"] = Json{{"file", config.at("typescript")}, {"format", "settings"}};
        } else if (backend == "model") {
            if (!external && config.contains("typescript")) throw std::runtime_error("model backend cannot request TypeScript output");
        } else {
            throw std::runtime_error("unknown backend " + backend);
        }
    }
    if (auto cpp = config.find("cpp"); cpp != config.end() && cpp->is_object()) {
        cpp->erase("emitEmptySource");
        // Older workspaces used booleans to toggle the forward header, or a string to name it.
        const auto forward = cpp->find("forward");
        if (forward != cpp->end() && forward->is_boolean()) cpp->erase(forward);
    }
    checkKeys(config, {"spec", "cpp", "typescript", "types", "runtimeFiles"}, "module");
    return config;
}

std::string bindingName(const std::string& qualified) {
    const auto pos = qualified.rfind("::");
    return pos == std::string::npos ? qualified : qualified.substr(pos + 2);
}

[[noreturn]] void invalidBinding(const TypeScriptBinding& rule, std::string_view format) {
    throw std::runtime_error(rule.source.location.describe() + ": unsupported " + std::string(format) + " binding for " + rule.source.name);
}

SettingsBinding settingsBinding(const Module& module) {
    SettingsBinding result;
    for (const auto& rule : module.bindings.typescript) {
        if (rule.kind == BindingTarget::type) continue;
        const auto name = bindingName(rule.source.name);
        if (rule.kind == BindingTarget::constEnum || rule.kind == BindingTarget::classType ||
            rule.kind == BindingTarget::numberField || rule.names || !rule.methods.empty() || !rule.callbacks.empty())
            invalidBinding(rule, "settings");
        if (rule.kind == BindingTarget::interfaceType && !rule.helpers) result.interfaceOnly.insert(name);
        if (rule.registerEnum) result.registerEnums.insert(name);
        if (!rule.renamedFrom.empty()) result.renamedFrom.emplace(name, rule.renamedFrom);
    }
    return result;
}

InterfaceBindings interfaceBindings(const Module& module) {
    InterfaceBindings result;
    for (const auto& rule : module.bindings.typescript) {
        for (const auto& [method, binding] : rule.methods)
            result.emplace(std::make_pair(bindingName(rule.source.name), method), binding);
    }
    return result;
}

void loadCppOutput(RecordOutput& output, const Json& cpp, const Json& defaults, const Module& module) {
    checkKeys(cpp, {"root", "dir", "prefix", "header", "forward", "names", "serialization", "jsb",
                    "forwardIncludes"}, "cpp");
    output.nativeRoot = outputPath(cpp.value("root", defaults.value("root", std::string{"."})));
    const auto dir = outputPath(cpp.value("dir", defaults.value("dir", std::string{"."})));
    const auto prefix = cpp.value("prefix", module.name);
    if (!identifier(prefix)) throw std::runtime_error("invalid file prefix " + prefix);
    std::set<fs::path> paths;
    const auto header = [&](const Json& filename) {
        auto path = (dir / outputPath(filename)).lexically_normal();
        if (path.extension() != ".h" || !paths.insert(path).second)
            throw std::runtime_error("expected a distinct .h output: " + path.generic_string());
        return path;
    };
    output.header = header(cpp.value("header", prefix + "Types.h"));
    output.forwardHeader = header(cpp.value("forward", prefix + "Fwd.h"));
    const auto optionalHeader = [&](const char* key, const char* suffix) -> std::optional<fs::path> {
        const auto value = cpp.value(key, Json(false));
        if (!value.is_boolean()) return header(value);
        if (value.get<bool>()) return header(prefix + suffix);
        return {};
    };
    output.namesHeader = optionalHeader("names", "Names.h");
    output.serializationHeader = optionalHeader("serialization", "Serialization.h");
    output.jsbHeader = optionalHeader("jsb", "Jsb.h");
    if (cpp.contains("forwardIncludes")) {
        if (!cpp.at("forwardIncludes").is_array())
            throw std::runtime_error("forwardIncludes requires an array of include paths");
        for (const auto& include : cpp.at("forwardIncludes")) {
            auto path = outputPath(include);
            if (path.extension() != ".h" || path == output.header || path == output.forwardHeader)
                throw std::runtime_error("expected a distinct .h forward include");
            output.forwardIncludes.insert(path.generic_string());
        }
    }
}

void bindRecordOutput(RecordOutput& output, const Module& module) {
    const auto& bindings = module.bindings;
    if (output.jsbHeader && !bindings.jsb)
        throw std::runtime_error(module.name + ": cpp.jsb requires a .spec binding jsb block");
    output.jsbIncludes = bindings.jsbIncludes;
    for (const auto& [source, location] : bindings.jsbRecords) output.jsbRecords.insert(bindingName(source));
    if (!output.typescript) return;
    output.poolModule = bindings.poolModule;
    output.interfaceBindings = interfaceBindings(module);
    std::set<std::string> poolMembers;
    for (const auto& rule : bindings.typescript) {
        if (rule.kind == BindingTarget::type) continue;
        const auto name = bindingName(rule.source.name);
        if (rule.registerEnum || rule.helpers || !rule.renamedFrom.empty())
            invalidBinding(rule, "records");
        if (rule.names) output.namedEnums.insert(name);
        if (rule.kind == BindingTarget::constEnum) output.constEnums.insert(name);
        if (rule.reset) output.resets.emplace(name, *rule.reset);
        if (!rule.fieldResets.empty()) {
            if (!rule.reset && !rule.pool)
                throw std::runtime_error(rule.source.location.describe() + ": field reset requires a reset method on " + name);
            output.fieldResets.emplace(name, rule.fieldResets);
        }
        if (rule.base) output.bases.emplace(name, *rule.base);
        if (rule.kind == BindingTarget::classType && !rule.from.empty()) {
            if (rule.base || rule.constructor || rule.reset || !rule.fieldDefaults.empty() || !rule.fieldResets.empty() ||
                (rule.pool && (rule.pool->assignFields || !rule.pool->keepOnReset.empty()))) invalidBinding(rule, "externally implemented class");
            output.externalClasses.emplace(name, rule.from);
        }
        if (rule.pool) {
            if (rule.pool->assignFields && (rule.reset || !rule.fieldResets.empty()))
                throw std::runtime_error(rule.source.location.describe() + ": pool assign conflicts with reset");
            if (output.poolModule.empty()) throw std::runtime_error(rule.source.location.describe() + ": pool binding requires poolModule");
            if (!poolMembers.insert(rule.pool->member).second)
                throw std::runtime_error(rule.source.location.describe() + ": duplicate pool member " + rule.pool->member);
            output.pools.emplace(name, *rule.pool);
        }
    }
    if (output.pools.empty() && !bindings.poolDependencies.empty())
        throw std::runtime_error(module.name + ": poolDependency requires a local object pool");
    for (const auto& [member, dependency] : bindings.poolDependencies) {
        if (!poolMembers.insert(member).second) throw std::runtime_error(module.name + ": duplicate pool member " + member);
    }
    for (const auto& name : {"constructor", "reset"})
        if (poolMembers.contains(name)) throw std::runtime_error(module.name + ": conflicting pool member " + name);
    for (const auto& [name, pool] : output.pools)
        if (poolMembers.contains("create" + name)) throw std::runtime_error(module.name + ": conflicting pool member create" + name);
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
    TypeTable types;
    if (settings.contains("types")) types = loadTypes(object(base / settings.at("types").get<std::string>()));
    auto preamble = normalizeLines(read(base / settings.at("preamble").get<std::string>()), false);
    const auto eol = settings.at("lineEnding").get<std::string>();
    if (eol != "crlf" && eol != "lf") throw std::runtime_error("lineEnding must be lf or crlf");
    const auto& modules = settings.at("modules");
    if (!modules.is_array() || modules.empty()) throw std::runtime_error("expected modules array");
    const auto defaults = settings.value("cppDefaults", Json::object());
    checkKeys(defaults, {"root", "dir"}, "cppDefaults");
    std::vector<ModuleInput> inputs;
    std::vector<ModuleOutput> outputs;
    for (const auto& entry : modules) {
        const auto spec = base / entry.at("spec").get<std::string>();
        auto module = parseModule(spec, read(spec));
        const auto config = moduleConfig(entry, module.external);
        auto& output = outputs.emplace_back();
        auto& typesOutput = output.types;
        if (module.external) {
            if (config.contains("cpp")) throw std::runtime_error(spec.string() + ": extern module cannot declare generated outputs");
            if (const auto ts = config.find("typescript"); ts != config.end()) {
                if (!ts->is_string()) throw std::runtime_error(spec.string() + ": extern module typescript expects an import path string");
                auto name = ts->get<std::string>();
                if (name.ends_with(".d.ts")) name.resize(name.size() - 5);
                else if (name.ends_with(".ts")) name.resize(name.size() - 3);
                module.bindings.typescriptFrom = parseBindingPath(name, spec.string());
            }
            for (const auto& rule : module.bindings.typescript)
                if (rule.kind != BindingTarget::type) invalidBinding(rule, "extern module");
        }
        if (module.external || (!config.contains("cpp") && !config.contains("typescript"))) {
            inputs.push_back({std::move(module), loadTypes(config.value("types", Json::object())), true});
            continue;
        }
        if (config.contains("types")) throw std::runtime_error(spec.string() + ": types mappings require a model-only module");
        if (config.contains("cpp")) loadCppOutput(typesOutput, config.at("cpp"), defaults, module);
        fs::path tsModule;
        if (config.contains("typescript")) {
            const auto& ts = config.at("typescript");
            if (!ts.is_string()) checkKeys(ts, {"file", "format"}, "typescript");
            const auto path = outputPath(ts.is_string() ? ts : ts.at("file"));
            if (path.extension() != ".ts") throw std::runtime_error("expected .ts output");
            const auto format = ts.is_string() ? "types" : ts.value("format", std::string{"types"});
            if (format == "settings") output.settingsFile = path;
            else if (format == "types") typesOutput.typescript = path;
            else throw std::runtime_error("unknown TypeScript format " + format);
            tsModule = path;
            tsModule.replace_extension();
        }
        bindRecordOutput(typesOutput, module);
        TypeTable mappings;
        for (const auto& decl : module.declarations) {
            if (std::holds_alternative<AliasDeclaration>(decl.data)) {
                mappings.emplace(decl.name, ExternalType{.header = typesOutput.forwardHeader.generic_string()});
            } else if (std::holds_alternative<TagDeclaration>(decl.data)) {
                mappings.emplace(decl.name, ExternalType{.header = typesOutput.header.generic_string()});
            } else {
                // Settings records export interfaces and factories, while ordinary records export constructors.
                const bool settingsRecord = output.settingsFile && std::holds_alternative<Record>(decl.data);
                mappings.emplace(decl.name, ExternalType{.header = typesOutput.header.generic_string(),
                    .typescript = tsModule.empty() ? std::nullopt : std::optional{decl.name},
                    .typescriptModule = settingsRecord ? std::string{} : tsModule.generic_string()});
            }
        }
        inputs.push_back({std::move(module), std::move(mappings)});
    }
    std::vector<ModuleId> dependencyOrder;
    const auto graph = buildTypeGraph(inputs, types, &dependencyOrder);
    std::set<std::string> outputKeys;
    Files files;
    const auto addFile = [&](const fs::path& path, const std::string& content) {
        auto relative = path.lexically_normal();
        auto key = relative.generic_string();
        std::ranges::transform(key, key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (!outputKeys.insert(key).second) throw std::runtime_error("duplicate output " + key);
        files.emplace(relative, normalizeLines(content, eol == "crlf"));
    };
    NestedResets nestedResets;
    for (const auto owner : dependencyOrder) {
        const auto i = owner.value;
        const auto& syntax = inputs[i].syntax;
        const auto& module = graph.modules.at(i);
        auto& output = outputs[i];
        auto& records = output.types;
        if (output.settingsFile)
            addFile(*output.settingsFile, emitSettings(graph, owner, settingsBinding(syntax), *output.settingsFile, preamble));
        if (!records.header.empty() || records.typescript) {
            if (records.typescript) {
                for (const auto& [member, name] : syntax.bindings.poolDependencies) {
                    const auto found = std::ranges::find_if(module.imports, [&](ModuleId id) { return graph.modules.at(id.value).name == name; });
                    if (found == module.imports.end()) throw std::runtime_error(module.name + ": poolDependency requires import " + name);
                    const auto& dependency = outputs.at(found->value).types;
                    if (dependency.pools.empty() || !dependency.typescript)
                        throw std::runtime_error(module.name + ": imported module has no object pool: " + name);
                    auto from = *dependency.typescript;
                    records.poolDependencies.emplace_back(member, TypeScriptImport{name + "ObjectPool", from.replace_extension()});
                }
            }
            for (auto imported : module.imports) {
                const auto& dependency = outputs.at(imported.value).types;
                if (!records.forwardHeader.empty() && !dependency.forwardHeader.empty())
                    records.forwardIncludes.insert(dependency.forwardHeader.generic_string());
                if (records.namesHeader && dependency.namesHeader)
                    records.importedNamesHeaders.push_back(*dependency.namesHeader);
            }
            const auto emitted = emitRecords(graph, owner, records, resolveArchive(graph, owner, outputs), preamble, nestedResets);
            for (const auto& [path, content] : emitted) addFile(path, content);
        }
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
