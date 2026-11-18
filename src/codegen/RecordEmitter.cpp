#include "codegen/RecordEmitter.h"
#include "codegen/EnumEmitter.h"
#include "codegen/TypeScriptImports.h"
#include "generated/TypeGraphTypes.h"
#include "model/IntegerLiteral.h"

#include <algorithm>
#include <locale>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace cc::codegen {
namespace {

struct Dependencies {
    std::set<std::string> headers;
    std::set<std::string> forwardDeclarations;
    TypeImports typeImports;
    TypeImports valueImports;
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
    bool usesAllocator{};
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
    if (type != "float" && type != "double") return validUnsignedFieldDefault(type, literal);
    if (literal.empty()) return false;
    const auto first = literal.front() == '-' ? 1U : 0U;
    if (first == literal.size() || literal[first] < '0' || literal[first] > '9') return false;
    if (literal[first] == '0' && first + 1 < literal.size() && literal[first + 1] != '.') return false;
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

Member projectMember(const TypeGraph& graph, ModuleId owner, const FieldInfo& field, Dependencies& deps,
                     bool emitTs) {
    const auto& module = graph.modules.at(owner.value);
    const auto& node = graph.node(field.type.type);
    Member m{.name = field.name, .cppDefault = field.initializer, .readonly = field.isReadonly};
    if (node.qualifiedName == "ccstd::pmr::string") {
        if (field.isNullable || !field.initializer.empty()) invalid(field, "expected a default-constructed PMR string");
        if (node.cppHeader.empty()) invalid(field, "missing native header for " + node.qualifiedName);
        m.cpp = nativeType(graph, module, field.type.type, deps);
        if (emitTs) {
            if (node.typescriptName != "string") invalid(field, "expected a TypeScript string mapping for " + node.qualifiedName);
            m.ts = *node.typescriptName;
            m.tsDefault = "''";
        }
        m.move = true;
        m.usesAllocator = true;
        return m;
    }
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
    if (std::holds_alternative<EnumType>(node.data)) {
        const auto pos = field.initializer.rfind("::");
        if (pos == std::string::npos) invalid(field, "expected a named enum default");
        const auto member = field.initializer.substr(pos + 2);
        m.cppDefault = m.cpp + "::" + member;
        m.tsDefault = m.ts + "." + member;
        if (node.module != owner) {
            if (node.cppHeader.empty() || node.typescriptModule.empty()) invalid(field, "missing external enum mapping");
            deps.valueImports[node.typescriptModule].insert(m.ts);
        }
        return m;
    }
    if (m.ts == "boolean" && (m.tsDefault == "false" || m.tsDefault == "true")) return m;
    if (m.ts == "number" && numericDefault(node.qualifiedName, m.tsDefault)) return m;
    invalid(field, "unsupported scalar default " + field.initializer);
}

using Order = std::vector<uint32_t>;

void cppFlagOperators(std::ostream& out, const std::string& name, const std::string& base) {
    for (const char op : {'|', '&'}) {
        out << "\nconstexpr " << name << " operator" << op << "(const " << name << " lhs, const " << name << " rhs) noexcept {\n"
            << "    return static_cast<" << name << ">(static_cast<" << base << ">(lhs) " << op
            << " static_cast<" << base << ">(rhs));\n}\n";
    }
    for (const char op : {'|', '&'}) {
        out << "\nconstexpr " << name << "& operator" << op << "=(" << name << "& lhs, const " << name << " rhs) noexcept {\n"
            << "    return lhs = lhs " << op << " rhs;\n}\n";
    }
    out << "\nconstexpr bool operator!(" << name << " e) noexcept {\n"
        << "    return e == static_cast<" << name << ">(0);\n}\n"
        << "\nconstexpr " << name << " operator~(" << name << " e) noexcept {\n"
        << "    return static_cast<" << name << ">(~static_cast<std::underlying_type_t<" << name << ">>(e));\n}\n"
        << "\nconstexpr bool any(" << name << " e) noexcept {\n    return !!e;\n}\n";
}

void tsEnumNames(std::ostream& out, const std::string& name, const EnumType& enumeration) {
    out << "\nexport function get" << name << "Name (e: " << name << "): string {\n"
        << "    switch (e) {\n";
    std::set<decltype(Enumerator::value)> seen;
    for (const auto& value : enumeration.values) {
        if (!seen.insert(value.value).second) continue;
        out << "    case " << name << "." << value.name << ":\n"
            << "        return '" << value.name << "';\n";
    }
    out << "    default:\n        return '';\n    }\n}\n";
}

void inlineConstructors(std::ostream& out, const std::string& name, const RecordType& record,
                        const std::vector<Member>& members) {
    if (!record.constructors.empty()) out << "    " << name << "() = default;\n";
    for (const auto& ctor : record.constructors) {
        if (ctor.empty()) continue;
        out << "    " << name << "(";
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
}

enum class CtorArgs { defaults, fields, move, copy };

void pmrInitializers(std::ostream& source, const std::vector<Member>& members, CtorArgs args, const Order& fields = {}) {
    bool first = true;
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        const bool hasArg = args == CtorArgs::move || args == CtorArgs::copy ||
                            (args == CtorArgs::fields && std::ranges::find(fields, i) != fields.end());
        if (!m.usesAllocator && !hasArg) continue;
        source << (first ? "\n: " : ",\n  ") << m.name << "(";
        if (hasArg) {
            const bool move = m.move && args != CtorArgs::copy;
            if (move) source << "std::move(";
            source << (args == CtorArgs::fields ? m.name + "In" : "rhs." + m.name);
            if (move) source << ")";
            if (m.usesAllocator) source << ", ";
        }
        if (m.usesAllocator) source << "alloc";
        source << ")";
        first = false;
    }
    source << " {}\n";
}

void pmrConstructors(std::ostream& header, std::ostream& source, const std::string& name,
                     const RecordType& record, const std::vector<Member>& members, const Member& allocatorMember) {
    const std::string alloc = "const allocator_type& alloc";
    const auto defaultAlloc = alloc + " = boost::container::pmr::get_default_resource()";
    const auto constructor = "\n" + name + "::" + name + "(";
    header << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n"
           << "    allocator_type get_allocator() const noexcept { // NOLINT\n"
           << "        return {" << allocatorMember.name << ".get_allocator().resource()};\n    }\n\n"
           << "    " << name << "(" << defaultAlloc << ") noexcept; // NOLINT\n";
    source << constructor << alloc << ") noexcept";
    pmrInitializers(source, members, CtorArgs::defaults);
    for (const auto& order : record.constructors) {
        if (order.empty()) continue;
        std::string params;
        for (auto index : order) {
            const auto& m = members.at(index);
            params += m.cpp + " " + m.name + "In, ";
        }
        header << "    " << name << "(" << params << defaultAlloc << ");\n";
        source << constructor << params << alloc << ") // NOLINT";
        pmrInitializers(source, members, CtorArgs::fields, order);
    }
    for (auto args : {CtorArgs::move, CtorArgs::copy}) {
        const auto rhs = name + (args == CtorArgs::move ? "&& rhs" : " const& rhs");
        header << "    " << name << "(" << rhs << ", " << alloc << ");\n";
        source << constructor << rhs << ", " << alloc << ")";
        pmrInitializers(source, members, args);
    }
    header << "\n    " << name << "(" << name << "&& rhs) noexcept = default;\n"
           << "    " << name << "(" << name << " const& rhs) = delete;\n"
           << "    " << name << "& operator=(" << name << "&& rhs) noexcept = default;\n"
           << "    " << name << "& operator=(" << name << " const& rhs) = default;\n\n";
}

void cppEquality(std::ostream& out, const std::string& name, const std::vector<Member>& members) {
    const auto params = "(const " + name + "& lhs, const " + name + "& rhs) noexcept {\n";
    out << "\ninline bool operator==" << params;
    auto writeTuple = [&](std::string_view object) {
        out << "std::forward_as_tuple(";
        for (size_t i = 0; i < members.size(); ++i) {
            if (i) out << ", ";
            out << object << "." << members[i].name;
        }
        out << ")";
    };
    out << "    return ";
    writeTuple("lhs");
    out << " ==\n           ";
    writeTuple("rhs");
    out << ";\n}\n\ninline bool operator!=" << params << "    return !(lhs == rhs);\n}\n";
}

void cppRecord(std::ostream& out, std::ostream& source, const TypeGraph& graph, const TypeModule& module,
               const TypeNode& node, const std::vector<Member>& members, Dependencies& deps) {
    const auto& record = std::get<RecordType>(node.data);
    out << "struct " << node.name << " {\n";
    const auto allocator = std::ranges::find(members, true, &Member::usesAllocator);
    if (allocator == members.end()) {
        inlineConstructors(out, node.name, record, members);
    } else {
        pmrConstructors(out, source, node.name, record, members, *allocator);
    }
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
        if (!defaults || (m.ts != "number" && m.ts != "boolean" && m.ts != "string")) params += ": " + m.ts;
        if (defaults) params += " = " + m.tsDefault;
    }
    return params;
}

void tsRecord(std::ostream& ts, const std::string& name, const std::vector<Member>& members, const Order& order, bool pooled) {
    const auto params = tsParams(members, order, true, ", ");
    const bool multiline = params.size() + std::string_view("    constructor () {").size() > 120;
    auto signature = [&](bool defaults) {
        if (multiline) return "\n        " + tsParams(members, order, defaults, ",\n        ") + ",\n    ";
        return defaults ? params : tsParams(members, order, false, ", ");
    };
    ts << "export class " << name << " {\n" << "    constructor (" << signature(true) << ") {\n";
    auto assignments = [&] {
        for (const auto& m : members) ts << "        this." << m.name << " = " << m.name << ";\n";
        ts << "    }\n";
    };
    assignments();
    if (pooled) {
        ts << "    reset (" << signature(false) << "): void {\n";
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
    std::ostringstream cpp, definitions, ts, nameFunctions;
    auto pendingEnums = output.namedEnums;
    auto pendingConstEnums = output.constEnums;
    auto pendingComparable = output.comparable;
    std::string enumImports;
    size_t poolCount{};
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        cpp << '\n';
        ts << '\n';
        if (const auto* enumeration = std::get_if<EnumType>(&node.data)) {
            cpp << "enum class " << node.name;
            std::string baseName = "int";
            if (enumeration->underlying != TypeId{}) {
                const auto& base = graph.node(enumeration->underlying);
                if (!base.cppHeader.empty()) deps.headers.insert(base.cppHeader);
                baseName = cppName(graph, base, module);
                cpp << " : " << baseName;
            }
            cpp << (enumeration->isFlags ? " { // NOLINT(performance-enum-size)\n" : " {\n");
            ts << (pendingConstEnums.erase(node.name) ? "export const enum " : "export enum ") << node.name << " {\n";
            writeEnumMembers(cpp, *enumeration, EnumTarget::cpp);
            if (output.typescript) writeEnumMembers(ts, *enumeration, EnumTarget::typescript);
            cpp << "};\n";
            if (enumeration->isFlags) {
                deps.headers.insert("type_traits");
                cppFlagOperators(cpp, node.name, baseName);
            }
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
        for (const auto& field : record->fields) members.push_back(projectMember(graph, owner, field, deps, output.typescript.has_value()));
        cppRecord(cpp, definitions, graph, module, node, members, deps);
        if (pendingComparable.erase(node.name)) {
            deps.headers.insert("tuple");
            cppEquality(cpp, node.name, members);
        }
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
    if (!pendingComparable.empty()) throw std::runtime_error("unknown comparable record " + *pendingComparable.begin() + " in " + module.name);
    if (!pendingEnums.empty()) throw std::runtime_error("unknown enum " + *pendingEnums.begin() + " in " + module.name);
    if (!pendingConstEnums.empty()) throw std::runtime_error("unknown const enum " + *pendingConstEnums.begin() + " in " + module.name);

    std::ostringstream header, script;
    header << preamble << "#pragma once\n";
    for (const auto& include : deps.headers) {
        if (include.find('/') == std::string::npos) header << "#include <" << include << ">\n";
        else header << "#include \"" << include << "\"\n";
    }
    for (const auto& decl : deps.forwardDeclarations) header << "\n" << decl << "\n";
    auto writeNamespace = [&](std::ostream& out, const std::ostringstream& body) {
        for (const auto& ns : module.namespaces) out << "\nnamespace " << ns << " {\n";
        out << body.str();
        for (auto it = module.namespaces.rbegin(); it != module.namespaces.rend(); ++it) out << "\n} // namespace " << *it << "\n";
    };
    writeNamespace(header, cpp);

    Files files{{output.header, header.str()}};
    if (!definitions.view().empty()) {
        auto path = output.header;
        path.replace_extension(".cpp");
        std::ostringstream source;
        source << preamble << "#include \"" << output.header.filename().generic_string() << "\"\n#include <utility>\n";
        writeNamespace(source, definitions);
        files.emplace(path, source.str());
    }
    if (!output.typescript) return files;
    script << preamble << "/* eslint-disable max-len */\n";
    writeImports(script, deps.valueImports, *output.typescript, false);
    writeImports(script, deps.typeImports, *output.typescript, true);
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
