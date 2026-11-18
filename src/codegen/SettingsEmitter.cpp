#include "codegen/SettingsEmitter.h"
#include "codegen/EnumEmitter.h"
#include "generated/TypeGraphTypes.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace cc::codegen {
namespace {

struct Imports {
    ModuleId owner;
    std::set<std::string> names;
    std::map<std::string, std::string> aliases;
};

enum class FieldKind { scalar, resource, record };

struct ProjectedField {
    FieldKind kind;
    std::string type;
    std::string initial;
};

[[noreturn]] void invalid(const FieldInfo& field, const std::string& message) {
    throw std::runtime_error(field.type.location.describe() + ": " + field.name + ": " + message);
}

std::string typeName(const TypeNode& type, const FieldInfo& field, Imports& imports) {
    if (!type.typescriptName) invalid(field, "missing TypeScript mapping for " + type.qualifiedName);
    if (type.module != ModuleId{} && type.module != imports.owner) {
        invalid(field, "missing TypeScript import mapping for " + type.qualifiedName);
    }
    const auto& name = *type.typescriptName;
    if (type.module == ModuleId{} && name != "boolean" && name != "number" && name != "string") {
        imports.names.insert(name.substr(0, name.find('.')));
    }
    return name;
}

ProjectedField project(const TypeGraph& graph, const FieldInfo& field, Imports& imports) {
    const auto& node = graph.node(field.type.type);
    if (field.isNullable) {
        const auto* ptr = std::get_if<TemplateInstanceType>(&node.data);
        if (!ptr || graph.node(ptr->declaration).qualifiedName != "cc::IntrusivePtr" ||
            ptr->arguments.size() != 1 || !std::holds_alternative<TypeId>(ptr->arguments.front()) ||
            field.initializer != "null") {
            invalid(field, "expected a nullable IntrusivePtr with a null default");
        }
        const auto& target = graph.node(std::get<TypeId>(ptr->arguments.front()));
        return {FieldKind::resource, typeName(target, field, imports) + " | null", "null"};
    }
    const auto name = typeName(node, field, imports);
    if (std::holds_alternative<RecordType>(node.data)) {
        if (!field.initializer.empty()) invalid(field, "nested settings use their record defaults");
        return {FieldKind::record, name, "make" + name + "()"};
    }
    const auto& initial = field.initializer;
    const auto pos = initial.rfind("::");
    if (pos != std::string::npos) {
        auto alias = name;
        if (const auto dot = name.rfind('.'); dot != std::string::npos) {
            alias = name.substr(dot + 1);
            const auto group = name.substr(0, dot);
            const auto [it, inserted] = imports.aliases.emplace(alias, group);
            if (!inserted && it->second != group) invalid(field, "ambiguous enum alias " + alias);
        }
        return {FieldKind::scalar, name, alias + "." + initial.substr(pos + 2)};
    }
    if (name == "boolean" && (initial == "true" || initial == "false")) return {FieldKind::scalar, name, initial};
    // The parser admits a signed decimal literal or a qualified identifier here.
    if (name == "number" && !initial.empty()) {
        const auto start = initial.front() == '-' ? 1U : 0U;
        if (start < initial.size() && std::isdigit(static_cast<unsigned char>(initial[start])) &&
            !(initial[start] == '0' && initial.size() > start + 1 && initial[start + 1] != '.')) {
            return {FieldKind::scalar, name, initial};
        }
    }
    invalid(field, "invalid default '" + initial + "' for " + name);
}

using Renames = std::map<std::string, std::string>;

void emitRecord(std::ostream& out, const TypeGraph& graph, const TypeNode& node, const SettingsProjection& rules,
                Imports& imports) {
    const auto& fields = std::get<RecordType>(node.data).fields;
    const bool helpers = !rules.interfaceOnly.contains(node.name);
    const auto group = rules.renamedFrom.find(node.name);
    const Renames empty;
    const auto& renames = group == rules.renamedFrom.end() ? empty : group->second;
    for (const auto& [to, from] : renames) {
        const auto dst = std::ranges::find(fields, to, &FieldInfo::name);
        const auto src = std::ranges::find(fields, from, &FieldInfo::name);
        if (!helpers || dst == fields.end() || src == fields.end() || dst == src || !dst->isNullable ||
            !src->isNullable || src->type.type != dst->type.type) {
            throw std::runtime_error("invalid settings migration " + node.name + "." + to + " from " + from);
        }
    }

    std::ostringstream factory, fill;
    out << "export interface " << node.name << " {\n";
    for (const auto& field : fields) {
        const auto p = project(graph, field, imports);
        out << "    ";
        if (p.kind == FieldKind::resource) out << "/* refcount */ ";
        if (field.isReadonly) out << "readonly ";
        out << field.name << ": " << p.type << ";";
        if (p.kind == FieldKind::scalar) out << " /* " << p.initial << " */";
        out << '\n';
        if (!helpers) continue;
        factory << "        " << field.name << ": " << p.initial << ",\n";
        const auto member = "value." + field.name;
        const auto target = field.isReadonly ? "(" + member + " as " + p.type + ")" : member;
        if (p.kind == FieldKind::record) {
            if (rules.interfaceOnly.contains(p.type)) invalid(field, "nested record requires helpers for " + p.type);
            fill << "    if (!" << member << ") {\n"
                 << "        " << target << " = " << p.initial << ";\n"
                 << "    } else {\n"
                 << "        fillRequired" << p.type << "(" << member << ");\n";
        } else {
            fill << "    if (" << member << " === undefined) {\n" << "        " << target << " = ";
            if (const auto it = renames.find(field.name); it != renames.end()) fill << "value." << it->second << " || ";
            fill << p.initial << ";\n";
        }
        fill << "    }\n";
    }
    out << "    [name: string]: unknown;\n}\n";
    if (helpers) {
        out << "\nexport function make" << node.name << "(): " << node.name << " {\n"
            << "    return {\n" << factory.str() << "    };\n}\n"
            << "\nexport function fillRequired" << node.name << "(value: " << node.name << "): void {\n"
            << fill.str() << "}\n";
    }
}

} // namespace

std::string emitSettings(const TypeGraph& graph, ModuleId owner, const SettingsProjection& rules,
                         const std::string& preamble) {
    const auto& module = graph.modules.at(owner.value);
    auto hasDeclaration = [&](const std::string& name, bool expectEnum) {
        return std::ranges::any_of(module.declarations, [&](TypeId id) {
            const auto& node = graph.node(id);
            return node.name == name && (expectEnum ? std::holds_alternative<EnumType>(node.data)
                                                    : std::holds_alternative<RecordType>(node.data));
        });
    };
    for (const auto& name : rules.interfaceOnly) {
        if (!hasDeclaration(name, false)) throw std::runtime_error("unknown interfaceOnly record " + name);
    }
    for (const auto& name : rules.registerEnums) {
        if (!hasDeclaration(name, true)) throw std::runtime_error("unknown enum registration " + name);
    }
    for (const auto& [name, fields] : rules.renamedFrom) {
        if (!hasDeclaration(name, false)) throw std::runtime_error("unknown migration record " + name);
    }

    Imports imports{.owner = owner};
    std::ostringstream body;
    for (const auto id : module.declarations) {
        const auto& node = graph.node(id);
        body << '\n';
        if (const auto* enumeration = std::get_if<EnumType>(&node.data)) {
            body << "export enum " << node.name << " {\n";
            writeEnumMembers(body, *enumeration);
            body << "}\n";
            if (rules.registerEnums.contains(node.name)) {
                imports.names.insert("ccenum");
                body << "ccenum(" << node.name << ");\n";
            }
        } else if (std::holds_alternative<RecordType>(node.data)) {
            emitRecord(body, graph, node, rules, imports);
        } else {
            throw std::runtime_error("unsupported settings declaration " + node.qualifiedName);
        }
    }

    std::ostringstream out;
    out << preamble << "/* eslint-disable max-len */\n";
    if (!imports.names.empty()) {
        out << "import { ";
        for (auto it = imports.names.begin(); it != imports.names.end(); ++it) {
            if (it != imports.names.begin()) out << ", ";
            out << *it;
        }
        out << " } from 'cc';\n";
    }
    if (!imports.aliases.empty()) out << '\n';
    for (const auto& [name, group] : imports.aliases) out << "const { " << name << " } = " << group << ";\n";
    return out.str() + body.str();
}

} // namespace cc::codegen
