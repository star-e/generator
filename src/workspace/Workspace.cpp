#include "workspace/Workspace.h"
#include "codegen/InterfaceEmitter.h"
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

TypeTable loadTypes(const fs::path& path) {
    TypeTable types;
    const auto json = object(path);
    for (const auto& [name, value] : json.items()) {
        ExternalType type{value.value("header", std::string{})};
        type.builtin = value.value("builtin", false);
        if (value.contains("ts")) type.typescript = value.at("ts").get<std::string>();
        types.emplace(name, std::move(type));
    }
    return types;
}

Projection loadProjection(const fs::path& path, const Module& module) {
    Projection projection;
    const auto json = object(path);
    for (const auto& [name, rules] : json.items()) {
        const auto decl = std::ranges::find(module.declarations, name, &Declaration::name);
        if (decl == module.declarations.end() || !std::holds_alternative<Interface>(decl->data) || !rules.is_object()) {
            throw std::runtime_error(path.string() + ": invalid interface rule " + name);
        }
        const auto& methods = std::get<Interface>(decl->data).methods;
        for (const auto& [method, value] : rules.items()) {
            if (std::ranges::find(methods, method, &Method::name) == methods.end()) {
                throw std::runtime_error(path.string() + ": unknown method " + name + "." + method);
            }
            auto& target = projection[name][method];
            if (!value.is_null()) {
                target = value.get<std::string>();
                if (!identifier(*target)) throw std::runtime_error(path.string() + ": invalid TS name " + *target);
            }
        }
    }
    return projection;
}

struct SettingsOutput {
    fs::path typescript;
    SettingsProjection projection;
};

SettingsProjection loadSettingsProjection(const fs::path& path) {
    const auto json = object(path);
    SettingsProjection projection;
    for (const auto& [key, value] : json.items()) {
        if (key == "interfaceOnly") value.get_to(projection.interfaceOnly);
        else if (key == "registerEnums") value.get_to(projection.registerEnums);
        else if (key == "renamedFrom") value.get_to(projection.renamedFrom);
        else throw std::runtime_error(path.string() + ": unknown settings projection " + key);
    }
    return projection;
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
    auto types = loadTypes(base / settings.at("types").get<std::string>());
    auto preamble = normalizeLines(read(base / settings.at("preamble").get<std::string>()), false);
    const auto eol = settings.at("lineEnding").get<std::string>();
    if (eol != "crlf" && eol != "lf") throw std::runtime_error("lineEnding must be lf or crlf");
    const auto& modules = settings.at("modules");
    if (!modules.is_array() || modules.empty()) throw std::runtime_error("expected modules array");
    std::vector<ModuleInput> inputs;
    std::vector<std::variant<OutputLayout, SettingsOutput>> layouts;
    for (const auto& config : modules) {
        const auto spec = base / config.at("spec").get<std::string>();
        auto module = parseModule(spec, read(spec));
        const auto projection = base / config.at("projection").get<std::string>();
        const auto ts = outputPath(config.at("typescript"));
        if (ts.extension() != ".ts") throw std::runtime_error("expected .ts output");
        const auto backend = config.value("backend", std::string{"interfaces"});
        if (backend == "settings") {
            layouts.emplace_back(SettingsOutput{ts, loadSettingsProjection(projection)});
            inputs.push_back({std::move(module), {}});
        } else if (backend == "interfaces") {
            OutputLayout layout{outputPath(config.at("nativeRoot")), outputPath(config.at("includeDir")),
                                config.at("prefix").get<std::string>(), ts};
            if (!identifier(layout.prefix)) throw std::runtime_error("invalid file prefix " + layout.prefix);
            auto methods = loadProjection(projection, module);
            layouts.emplace_back(std::move(layout));
            inputs.push_back({std::move(module), std::move(methods)});
        } else {
            throw std::runtime_error("unknown backend " + backend);
        }
    }
    const auto graph = buildTypeGraph(inputs, types);
    std::set<std::string> outputKeys;
    Files files;
    for (size_t i = 0; i < layouts.size(); ++i) {
        const ModuleId owner{static_cast<uint32_t>(i)};
        Files emitted;
        if (const auto* settingsOutput = std::get_if<SettingsOutput>(&layouts[i])) {
            emitted.emplace(settingsOutput->typescript, emitSettings(graph, owner, settingsOutput->projection, preamble));
        } else {
            emitted = emitInterfaces(graph, owner, std::get<OutputLayout>(layouts[i]), preamble);
        }
        for (auto& [path, content] : emitted) {
            auto relative = path.lexically_normal();
            auto key = relative.generic_string();
            std::ranges::transform(key, key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (!outputKeys.insert(key).second) throw std::runtime_error("duplicate output " + key);
            files.emplace(relative, normalizeLines(content, eol == "crlf"));
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
