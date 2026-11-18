#include "codegen/RecordEmitter.h"
#include "codegen/TypeScriptImports.h"
#include "generated/TypeGraphTypes.h"

#include <algorithm>
#include <charconv>
#include <locale>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>

namespace cc::codegen {
namespace {

struct Dependencies {
    std::set<std::string> headers;
    std::set<std::string> forwardDeclarations;
    TypeImports typeImports;
};

struct Member {
    std::string name;
    std::string cpp;
    std::string ts;
    std::string cppDefault;
    std::string tsDefault;
    std::string comment;
    bool move{};
    bool readonly{};
};

std::string cppName(const TypeGraph& graph, const TypeNode& type, const TypeModule& module) {
    std::string scope;
    std::string name = type.qualifiedName;
    for (const auto& ns : module.namespaces) {
        scope += ns + "::";
        if (type.qualifiedName.starts_with(scope)) name = type.qualifiedName.substr(scope.size());
    }
    // Qualify from the root if a closer declaration shadows the shortened name.
    scope.clear();
    for (auto it = module.namespaces.begin(); it != module.namespaces.end(); ++it) {
        scope += *it + "::";
        const auto other = graph.findType(scope + name);
        if (other != TypeId{} && graph.node(other).qualifiedName != type.qualifiedName) return "::" + type.qualifiedName;
    }
    return name;
}

[[noreturn]] void invalid(const FieldInfo& field, const std::string& reason) {
    throw std::runtime_error(field.type.location.describe() + ": " + field.name + ": " + reason);
}

bool numericDefault(const std::string& type, const std::string& literal) {
    // Signed decimal syntax is checked by the parser. Both targets reject legacy octal literals.
    if (literal.empty()) return false;
    const auto first = literal.front() == '-' ? 1U : 0U;
    if (first == literal.size() || literal[first] < '0' || literal[first] > '9') return false;
    if (literal[first] == '0' && first + 1 < literal.size() && literal[first + 1] != '.') return false;
    if (type == "uint8_t" || type == "uint32_t") {
        uint32_t value{};
        const auto end = literal.data() + literal.size();
        const auto parsed = std::from_chars(literal.data(), end, value);
        return parsed.ec == std::errc{} && parsed.ptr == end && (type != "uint8_t" || value <= UINT8_MAX);
    }
    if (type != "float" && type != "double") return false;
    std::istringstream in(literal);
    in.imbue(std::locale::classic());
    if (type == "float") {
        float value{};
        return (in >> value) && in.eof();
    }
    double value{};
    return (in >> value) && in.eof();
}

std::string nativeType(const TypeGraph& graph, const TypeModule& module, TypeId id, Dependencies& deps) {
    const auto& node = graph.node(id);
    if (const auto* ptr = std::get_if<PointerType>(&node.data)) return nativeType(graph, module, ptr->pointee, deps) + "*";
    if (const auto* instance = std::get_if<TemplateInstanceType>(&node.data)) {
        std::string name = nativeType(graph, module, instance->declaration, deps) + "<";
        for (size_t i = 0; i < instance->arguments.size(); ++i) {
            if (i) name += ", ";
            const auto& arg = instance->arguments[i];
            if (const auto* type = std::get_if<TypeId>(&arg)) name += nativeType(graph, module, *type, deps);
            else name += std::get<std::string>(arg);
        }
        return name + ">";
    }
    if (!node.cppHeader.empty()) deps.headers.insert(node.cppHeader);
    return cppName(graph, node, module);
}

Member projectMember(const TypeGraph& graph, const TypeModule& module, const FieldInfo& field, Dependencies& deps,
                     bool emitTs) {
    const auto& node = graph.node(field.type.type);
    Member m{.name = field.name, .cppDefault = field.initializer, .readonly = field.isReadonly};
    if (!emitTs && !field.isNullable) {
        m.cpp = nativeType(graph, module, field.type.type, deps);
        return m;
    }
    const auto* ptr = std::get_if<PointerType>(&node.data);
    const auto* instance = std::get_if<TemplateInstanceType>(&node.data);
    if (ptr || instance) {
        if (!field.isNullable || (!field.initializer.empty() && field.initializer != "nullptr" && field.initializer != "null")) {
            invalid(field, "expected a nullable resource with a null default");
        }
        TypeId target;
        if (ptr) {
            target = ptr->pointee;
        } else {
            const auto& wrapper = graph.node(instance->declaration);
            if (wrapper.qualifiedName != "cc::IntrusivePtr" || instance->arguments.size() != 1 ||
                !std::holds_alternative<TypeId>(instance->arguments[0])) invalid(field, "unsupported resource type");
            if (wrapper.cppHeader.empty()) invalid(field, "missing native header for " + wrapper.qualifiedName);
            deps.headers.insert(wrapper.cppHeader);
            deps.headers.insert("utility");
            target = std::get<TypeId>(instance->arguments[0]);
        }
        const auto& resource = graph.node(target);
        const auto* external = std::get_if<ExternalTypeNode>(&resource.data);
        if (!external) invalid(field, "expected an external resource");
        if (emitTs) {
            if (!resource.typescriptName || resource.typescriptModule.empty()) invalid(field, "missing external resource mapping");
            deps.typeImports[resource.typescriptModule].insert(*resource.typescriptName);
            m.ts = *resource.typescriptName + " | null";
            m.tsDefault = "null";
        }
        const auto name = cppName(graph, resource, module);
        if (ptr) {
            const auto ns = resource.qualifiedName.rfind("::");
            std::string decl = std::string(external->isStruct ? "struct " : "class ") + resource.name + ";";
            if (ns != std::string::npos) decl = "namespace " + resource.qualifiedName.substr(0, ns) + " { " + decl + " }";
            deps.forwardDeclarations.insert(decl);
            m.cpp = name + "*";
            m.cppDefault = "nullptr";
            m.comment = "/*pointer*/ ";
        } else {
            if (resource.cppHeader.empty()) invalid(field, "missing native header for " + resource.qualifiedName);
            deps.headers.insert(resource.cppHeader);
            m.cpp = cppName(graph, graph.node(instance->declaration), module) + "<" + name + ">";
            m.cppDefault.clear();
            m.comment = "/*refcount*/ ";
            m.move = true;
        }
        return m;
    }
    if (field.isNullable || !node.typescriptName) invalid(field, "unsupported scalar type " + node.qualifiedName);
    if (!node.cppHeader.empty()) deps.headers.insert(node.cppHeader);
    m.cpp = cppName(graph, node, module);
    m.ts = *node.typescriptName;
    m.tsDefault = field.initializer;
    if (m.ts == "boolean" && (m.tsDefault == "false" || m.tsDefault == "true")) return m;
    if (m.ts == "number" && numericDefault(node.qualifiedName, m.tsDefault)) return m;
    invalid(field, "unsupported scalar default " + field.initializer);
}

using Order = std::vector<uint32_t>;

void tsEnumNames(std::ostream& out, const std::string& name, const EnumType& enumeration) {
    out << "\nexport function get" << name << "Name (e: " << name << "): string {\n"
        << "    switch (e) {\n";
    for (const auto& value : enumeration.values) {
        out << "    case " << name << "." << value.name << ":\n"
            << "        return '" << value.name << "';\n";
    }
    out << "    default:\n        return '';\n    }\n}\n";
}

void cppRecord(std::ostream& out, const TypeGraph& graph, const TypeModule& module, const TypeNode& node,
               const std::vector<Member>& members, Dependencies& deps) {
    const auto& record = std::get<RecordType>(node.data);
    out << "struct " << node.name << " {\n";
    if (!record.constructors.empty()) out << "    " << node.name << "() = default;\n";
    for (const auto& ctor : record.constructors) {
        if (ctor.empty()) continue;
        out << "    " << node.name << "(";
        for (size_t i = 0; i < ctor.size(); ++i) {
            const auto& m = members.at(ctor[i]);
            if (i) out << ", ";
            out << m.cpp << " " << m.name << "In";
        }
        out << ") noexcept\n";
        bool first = true;
        for (uint32_t i = 0; i < members.size(); ++i) {
            if (std::ranges::find(ctor, i) == ctor.end()) continue;
            const auto& m = members[i];
            out << (first ? "    : " : ",\n      ") << m.name << "(";
            if (m.move) out << "std::move(";
            out << m.name << "In" << (m.move ? "))" : ")");
            first = false;
        }
        out << " {}\n";
    }
    if (!record.constructors.empty()) out << '\n';
    for (const auto& m : members) {
        out << "    " << m.cpp << " " << m.name;
        if (!m.cppDefault.empty()) out << "{" << m.cppDefault << "}";
        out << ";\n";
    }
    for (const auto& method : record.methods) {
        out << "    " << nativeType(graph, module, method.result.type, deps) << " " << method.name << "(";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            if (i) out << ", ";
            const auto& p = method.parameters[i];
            out << nativeType(graph, module, p.type.type, deps) << " " << p.name;
        }
        out << ")";
        if (method.isConst) out << " const";
        if (method.isNoexcept) out << " noexcept";
        out << ";\n";
    }
    out << "};\n";
}

std::string tsParams(const std::vector<Member>& members, const Order& order, bool defaults, const std::string& sep) {
    std::string params;
    for (const auto index : order) {
        const auto& m = members.at(index);
        if (!params.empty()) params += sep;
        params += m.name;
        if (!defaults || (m.ts != "number" && m.ts != "boolean")) params += ": " + m.ts;
        if (defaults) params += " = " + m.tsDefault;
    }
    return params;
}

void tsRecord(std::ostream& ts, const std::string& name, const std::vector<Member>& members, const Order& order, bool pooled) {
    ts << "export class " << name << " {\n"
       << "    constructor (" << tsParams(members, order, true, ", ") << ") {\n";
    auto assignments = [&] {
        for (const auto& m : members) ts << "        this." << m.name << " = " << m.name << ";\n";
        ts << "    }\n";
    };
    assignments();
    if (pooled) {
        ts << "    reset (" << tsParams(members, order, false, ", ") << "): void {\n";
        assignments();
    }
    for (const auto& m : members) {
        if (pooled && m.readonly) throw std::runtime_error(name + "." + m.name + ": reset requires a writable field");
        ts << "    declare " << (m.readonly ? "readonly " : "") << m.comment << m.name << ": " << m.ts << ";\n";
    }
    ts << "}\n";
}

struct Pool {
    std::ostringstream resets, factories, fields;

    void add(const std::string& name, const std::string& pool, const std::vector<Member>& members, const Order& order) {
        resets << "        this." << pool << ".reset(); // " << name << "\n";
        factories << "    create" << name << " (\n";
        if (!order.empty()) factories << "        " << tsParams(members, order, true, ",\n        ") << ",\n";
        factories << "    ): " << name << " {\n        const v = this." << pool << ".add(); // " << name << "\n        v.reset(";
        for (size_t i = 0; i < order.size(); ++i) {
            if (i) factories << ", ";
            factories << members.at(order[i]).name;
        }
        factories << ");\n        return v;\n    }\n";
        fields << "    private readonly " << pool << ": RecyclePool<" << name << "> = createPool(" << name << ");\n";
    }
};

} // namespace

Files emitRecords(const TypeGraph& graph, ModuleId owner, const RecordOutput& output, const std::string& preamble) {
    const auto& module = graph.modules.at(owner.value);
    Dependencies deps;
    Pool pool;
    std::ostringstream cpp, ts, nameFunctions;
    auto pendingEnums = output.namedEnums;
    auto pendingConstEnums = output.constEnums;
    std::string enumImports;
    size_t poolCount{};
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        cpp << '\n';
        ts << '\n';
        if (const auto* enumeration = std::get_if<EnumType>(&node.data)) {
            cpp << "enum class " << node.name;
            if (enumeration->underlying != TypeId{}) {
                const auto& base = graph.node(enumeration->underlying);
                if (!base.cppHeader.empty()) deps.headers.insert(base.cppHeader);
                cpp << " : " << cppName(graph, base, module);
            }
            cpp << " {\n";
            ts << (pendingConstEnums.erase(node.name) ? "export const enum " : "export enum ") << node.name << " {\n";
            for (const auto& value : enumeration->values) {
                for (auto* out : {&cpp, &ts}) *out << "    " << value.name << ",\n";
            }
            cpp << "};\n";
            ts << "}\n";
            if (pendingEnums.erase(node.name)) {
                if (!enumImports.empty()) enumImports += ", ";
                enumImports += node.name;
                tsEnumNames(nameFunctions, node.name, *enumeration);
            }
            continue;
        }
        const auto* record = std::get_if<RecordType>(&node.data);
        if (!record) throw std::runtime_error("expected record or enum: " + node.qualifiedName);
        std::vector<Member> members;
        for (const auto& field : record->fields) members.push_back(projectMember(graph, module, field, deps, output.typescript.has_value()));
        cppRecord(cpp, graph, module, node, members, deps);
        if (!output.typescript) continue;
        Order order(members.size());
        std::iota(order.begin(), order.end(), 0U);
        if (!record->constructors.empty()) {
            const auto ctor = std::ranges::find(record->constructors, members.size(), &Order::size);
            if (ctor == record->constructors.end()) throw std::runtime_error(node.qualifiedName + ": TS constructor requires all fields");
            order = *ctor;
        }
        const auto entry = output.pools.find(node.name);
        tsRecord(ts, node.name, members, order, entry != output.pools.end());
        if (entry != output.pools.end()) {
            ++poolCount;
            pool.add(node.name, entry->second, members, order);
        }
    }
    if (poolCount != output.pools.size()) throw std::runtime_error("unknown pool record in " + module.name);
    if (!pendingEnums.empty()) throw std::runtime_error("unknown enum " + *pendingEnums.begin() + " in " + module.name);
    if (!pendingConstEnums.empty()) throw std::runtime_error("unknown const enum " + *pendingConstEnums.begin() + " in " + module.name);

    std::ostringstream header, script;
    header << preamble << "#pragma once\n";
    for (const auto& include : deps.headers) {
        if (include.find('/') == std::string::npos) header << "#include <" << include << ">\n";
        else header << "#include \"" << include << "\"\n";
    }
    for (const auto& decl : deps.forwardDeclarations) header << "\n" << decl << "\n";
    for (const auto& ns : module.namespaces) header << "\nnamespace " << ns << " {\n";
    header << cpp.str();
    for (auto it = module.namespaces.rbegin(); it != module.namespaces.rend(); ++it) header << "\n} // namespace " << *it << "\n";

    Files files{{output.header, header.str()}};
    if (!output.typescript) return files;
    script << preamble << "/* eslint-disable max-len */\n";
    writeTypeImports(script, deps.typeImports, *output.typescript);
    if (poolCount) {
        script << "import { RecyclePool } from '" << modulePath(output.poolModule, *output.typescript) << "';\n";
        ts << "\nfunction createPool<T> (Constructor: new() => T): RecyclePool<T> {\n"
           << "    return new RecyclePool<T>(() => new Constructor(), 16);\n}\n"
           << "\nexport class " << module.name << "ObjectPool {\n    constructor () {\n    }\n    reset (): void {\n"
           << pool.resets.str() << "    }\n" << pool.factories.str() << pool.fields.str() << "}\n";
    }
    script << ts.str();
    files.emplace(*output.typescript, script.str());
    if (!enumImports.empty()) {
        const auto stem = output.typescript->stem().generic_string();
        const auto namesPath = output.typescript->parent_path() / (stem + "-names.ts");
        files.emplace(namesPath, preamble + "import { " + enumImports + " } from './" + stem + "';\n" + nameFunctions.str());
    }
    return files;
}

} // namespace cc::codegen
