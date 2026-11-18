#include "codegen/CocosInterfaceEmitter.h"

#include <set>
#include <sstream>
#include <stdexcept>

namespace cc::codegen {
namespace {

constexpr auto lintBegin = "// NOLINTBEGIN(misc-include-cleaner, bugprone-easily-swappable-parameters)\n";
constexpr auto lintEnd = "// NOLINTEND(misc-include-cleaner, bugprone-easily-swappable-parameters)\n";

const ExternalType& resolve(const TypeTable& types, const TypeRef& type) {
    const auto it = types.find(type.name);
    if (it == types.end()) throw std::runtime_error(type.location.describe() + ": unknown type " + type.name);
    return it->second;
}

std::string tsType(const TypeTable& types, const TypeRef& type) {
    const auto& name = resolve(types, type).typescript;
    if (type.pointer || !name) throw std::runtime_error(type.location.describe() + ": no TS mapping for " + type.name);
    return *name;
}

void cppMethod(std::ostream& out, const Method& method) {
    out << "    virtual " << method.result.name << (method.result.pointer ? " *" : " ") << method.name << '(';
    for (size_t i = 0; i < method.parameters.size(); ++i) {
        const auto& param = method.parameters[i];
        if (i) out << ", ";
        out << param.type.name << (param.type.pointer ? " *" : " ") << param.name;
    }
    out << ')';
    if (method.isConst) out << " const";
    if (method.isNoexcept) out << " noexcept";
    out << " = 0;\n";
}

void cppInterface(std::ostream& out, const Interface& decl) {
    const auto& name = decl.name;
    out << "class " << name << " {\npublic:\n"
        << "    " << name << "() noexcept = default;\n"
        << "    " << name << '(' << name << "&& rhs) = delete;\n"
        << "    " << name << '(' << name << " const& rhs) = delete;\n"
        << "    " << name << "& operator=(" << name << "&& rhs) = delete;\n"
        << "    " << name << "& operator=(" << name << " const& rhs) = delete;\n"
        << "    virtual ~" << name << "() noexcept = default;\n\n";
    for (const auto& method : decl.methods) cppMethod(out, method);
    out << "};\n\n";
}

void tsInterface(std::ostream& out, const Interface& decl, const TypeTable& types,
                 const Projection& projection) {
    std::set<std::string> emitted;
    out << "export interface " << decl.name << " {\n";
    for (const auto& method : decl.methods) {
        std::string name = method.name;
        if (auto rules = projection.find(decl.name); rules != projection.end()) {
            auto rule = rules->second.find(name);
            if (rule != rules->second.end()) {
                if (!rule->second) continue;
                name = *rule->second;
            }
        }
        if (!emitted.insert(name).second) throw std::runtime_error("duplicate TS method " + decl.name + "." + name);
        out << "    " << name << " (";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            const auto& param = method.parameters[i];
            if (i) out << ", ";
            out << param.name << ": " << tsType(types, param.type);
        }
        out << "): " << tsType(types, method.result) << ";\n";
    }
    out << "}\n";
}

} // namespace

Files emitInterfaces(const Module& module, const OutputLayout& layout, const TypeTable& types,
                     const Projection& projection, const std::string& preamble) {
    std::set<std::string> headers;
    auto collectHeader = [&](const TypeRef& type) {
        const auto& header = resolve(types, type).header;
        if (!header.empty()) headers.insert(header);
    };
    for (const auto& decl : module.interfaces) {
        for (const auto& method : decl.methods) {
            collectHeader(method.result);
            for (const auto& param : method.parameters) collectHeader(param.type);
        }
    }

    std::ostringstream fwd, header, impl, ts;
    const auto fwdName = layout.prefix + "Fwd.h";
    const auto typeName = layout.prefix + "Types.h";
    fwd << preamble << "#pragma once\n// clang-format off\n";
    header << preamble << "// clang-format off\n" << lintBegin << "#pragma once\n";
    for (const auto& include : headers) header << "#include <" << include << ">\n";
    header << "#include \"" << (layout.includeDir / fwdName).generic_string() << "\"\n";
    impl << preamble << "// clang-format off\n" << lintBegin << "#include \"" << typeName << "\"\n";
    ts << preamble << "/* eslint-disable max-len */\n";
    for (auto* out : {&fwd, &header, &impl}) {
        for (const auto& name : module.namespaces) *out << "\nnamespace " << name << " {\n";
        *out << '\n';
    }
    for (size_t i = 0; i < module.interfaces.size(); ++i) {
        const auto& decl = module.interfaces[i];
        fwd << "class " << decl.name << ";\n";
        cppInterface(header, decl);
        if (i) ts << '\n';
        tsInterface(ts, decl, types, projection);
    }
    fwd << '\n';
    for (auto* out : {&fwd, &header, &impl}) {
        for (auto it = module.namespaces.rbegin(); it != module.namespaces.rend(); ++it) {
            *out << "} // namespace " << *it << "\n\n";
        }
        if (out != &fwd) *out << lintEnd;
        *out << "// clang-format on\n";
    }
    const auto dir = layout.nativeRoot / layout.includeDir;
    if (layout.typescript.extension() != ".ts") throw std::runtime_error("expected .ts output");
    return {{dir / fwdName, fwd.str()}, {dir / typeName, header.str()},
            {dir / (layout.prefix + "Types.cpp"), impl.str()}, {layout.typescript, ts.str()}};
}

} // namespace cc::codegen
