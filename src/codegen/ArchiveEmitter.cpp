#include "codegen/ArchiveEmitter.h"
#include <algorithm>

namespace cc::codegen {

const std::string& archiveMethod(const TypeNode& api, const std::string& method) {
    const auto& methods = std::get<InterfaceType>(api.data).methods;
    for (const auto& member : methods) {
        if (member.name == method && member.typescriptName && member.accessor == AccessorKind::None) return *member.typescriptName;
    }
    throw std::runtime_error("missing TypeScript binding for " + api.qualifiedName + "." + method);
}

std::string archivePrimitive(const TypeGraph& graph, TypeId id) {
    const auto& node = graph.node(id);
    if (node.typescriptNumber) return "Number";
    if (const auto* alias = std::get_if<AliasType>(&node.data)) return archivePrimitive(graph, alias->target);
    if (node.qualifiedName == "bool") return "Bool";
    if (const auto* external = std::get_if<ExternalTypeNode>(&node.data); external && external->isString) return "String";
    std::string_view type = node.qualifiedName;
    if (const auto* enumeration = std::get_if<EnumType>(&node.data))
        type = enumeration->underlying == TypeId{} ? std::string_view{"int"} : graph.node(enumeration->underlying).qualifiedName;
    static constexpr std::string_view numbers[]{"int", "unsigned", "short", "char", "int8_t", "uint8_t",
        "int16_t", "uint16_t", "int32_t", "uint32_t", "float", "double"};
    return std::ranges::find(numbers, type) != std::end(numbers) ? "Number" : "";
}

void ArchiveRequirements::checkType(const TypeGraph& graph, ModuleId owner, TypeId id, const std::string& context, bool emitTs) {
    const auto& node = graph.node(id);
    if (!node.typescriptNumber && !archivePrimitive(graph, id).empty()) return;
    if (std::holds_alternative<ExternalTypeNode>(node.data) && !node.typescriptModule.empty()) return;
    if (const auto* record = std::get_if<RecordType>(&node.data); record && record->serializable && node.module == owner) return;
    if (node.typescriptContainer != ContainerKind::None) {
        needsReadSize |= emitTs;
        const auto& args = std::get<TemplateInstanceType>(node.data).arguments;
        if (node.typescriptContainer == ContainerKind::Map && graph.node(std::get<TypeId>(args.front())).typescriptMapKey == MapKeyKind::String)
            throw std::runtime_error(context + ": mapKey string has no archive encoding for the native key");
        if (node.typescriptContainer == ContainerKind::Array && archivePrimitive(graph, std::get<TypeId>(args.front())) == "Bool")
            needsBoolVectorLoad = true;
        for (const auto& arg : args) checkType(graph, owner, std::get<TypeId>(arg), context, emitTs);
        return;
    }
    throw std::runtime_error(context + ": unsupported archive field type " + node.qualifiedName);
}

void ArchiveScript::value(std::ostream& out, TypeId id, const std::string& expr, const std::string& indent,
                          bool declare, unsigned depth) const {
    const auto& node = graph.node(id);
    const auto primitive = archivePrimitive(graph, id);
    if (!primitive.empty()) {
        const auto& method = archiveMethod(api, (save ? "write" : "read") + primitive);
        if (save) out << indent << "a." << method << "(" << expr << ");\n";
        else out << indent << (declare ? "const " : "") << expr << " = a." << method << "();\n";
        return;
    }
    const auto& name = node.typescriptName.value();
    const bool array = node.typescriptContainer == ContainerKind::Array;
    if (declare) {
        if (array) out << indent << "const " << expr << ": " << name << " = [];\n";
        else out << indent << "const " << expr << " = new " << name << "();\n";
    }
    if (node.typescriptContainer == ContainerKind::None) {
        const auto codec = (save ? "save" : "load") + name;
        if (std::holds_alternative<ExternalTypeNode>(node.data)) {
            const auto& from = node.typescriptArchiveModule.empty() ? imports.defaultCodecModule : node.typescriptArchiveModule;
            imports.values[from].insert(codec);
            if (declare) imports.values[node.typescriptModule].insert(name);
        }
        out << indent << codec << "(a, " << expr << ");\n";
        return;
    }
    const auto& args = std::get<TemplateInstanceType>(node.data).arguments;
    const bool map = node.typescriptContainer == ContainerKind::Map;
    const auto keyType = std::get<TypeId>(args.front());
    const auto element = std::get<TypeId>(args.back());
    const bool json = map && graph.node(keyType).typescriptMapKey == MapKeyKind::Json;
    const auto key = "key" + std::to_string(depth);
    const auto item = "item" + std::to_string(depth);
    const auto inner = indent + "    ";
    if (save) {
        out << indent << "a." << archiveMethod(api, "writeNumber") << "(" << expr << (array ? ".length" : ".size") << ");\n"
            << indent << "for (const " << (map ? "[" + key + ", " + item + "]" : item) << " of " << expr << ") {\n";
        if (map) value(out, keyType, json ? "JSON.parse(" + key + ")" : key, inner, false, depth + 1);
        value(out, element, item, inner, false, depth + 1);
    } else {
        const auto i = (array ? "index" : "left") + std::to_string(depth);
        if (array) {
            out << indent << expr << ".length = readSize(a);\n"
                << indent << "for (let " << i << " = 0; " << i << " < " << expr << ".length; ++" << i << ") {\n";
        } else {
            out << indent << "for (let " << i << " = readSize(a); " << i << " > 0; --" << i << ") {\n";
        }
        if (map) value(out, keyType, key, inner, true, depth + 1);
        value(out, element, item, inner, true, depth + 1);
        out << inner << expr;
        if (array) out << "[" << i << "] = " << item << ";\n";
        else if (map) out << ".set(" << (json ? "JSON.stringify(" + key + ")" : key) << ", " << item << ");\n";
        else out << ".add(" << item << ");\n";
    }
    out << indent << "}\n";
}

void emitGraphArchive(const TypeGraph& graph, const TypeNode& node, const TypeModule& archive,
                      std::ostream& declarations, std::ostream& cpp, std::ostream* ts, ArchiveImports& imports,
                      ArchiveRequirements& requirements) {
    const auto& model = std::get<GraphType>(node.data);
    const auto& name = node.name;
    if (!model.vertexList || !model.mutableGraph || !model.hierarchy || model.separateEdges || model.alternatives.empty() ||
        graph.node(model.vertexType).qualifiedName != "uint32_t")
        throw std::runtime_error(node.qualifiedName + ": graph archive requires a mutable uint32_t vertex list with shared edges and polymorphic payloads");
    requirements.needsReadSize |= ts != nullptr;
    for (const auto& columns : {&model.components, &model.alternatives})
        for (const auto& column : *columns) requirements.checkType(graph, node.module, column.valueType, node.qualifiedName + "." + column.name, ts != nullptr);
    for (const auto& field : model.fields) {
        if (field.transient) continue;
        if (field.isReadonly && !archivePrimitive(graph, field.type.type).empty())
            throw std::runtime_error(node.qualifiedName + "." + field.name + ": archive load requires a writable field");
        requirements.checkType(graph, node.module, field.type.type, node.qualifiedName + "." + field.name, ts != nullptr);
    }
    const auto tag = [&](const ComponentInfo& column) { return "::" + graph.node(column.tag).qualifiedName + "{}"; };
    const auto count = model.alternatives.size();
    const auto& output = graph.node(archive.outputArchive);
    const auto& input = graph.node(archive.inputArchive);
    const auto archiveName = [&](const TypeNode& api) {
        return graph.modules.at(node.module.value).namespaces == archive.namespaces ? api.name : "::" + api.qualifiedName;
    };
    const std::string save = "void save(" + archiveName(output) + "& ar, const " + name + "& g)";
    const std::string load = "void load(" + archiveName(input) + "& ar, " + name + "& g)";
    const bool takesName = model.nameComponentIndex || (!model.components.empty() &&
        graph.node(model.components.front().valueType).qualifiedName == "ccstd::pmr::string");
    declarations << '\n' << save << ";\n" << load << ";\n";
    cpp << '\n' << save << " {\n"
        << "    uint32_t edges = 0, counts[" << count << "]{};\n"
        << "    for (auto v : g.vertices()) {\n        edges += g.out_degree(v);\n"
        << "        ++counts[static_cast<uint32_t>(g.kind(v))];\n    }\n"
        << "    save(ar, g.num_vertices());\n    save(ar, edges);\n    for (auto n : counts) save(ar, n);\n"
        << "    for (auto v : g.vertices()) {\n        save(ar, static_cast<uint32_t>(g.kind(v)));\n        save(ar, g.get_parent(v));\n";
    for (const auto& c : model.components) cpp << "        save(ar, g.get" << c.publicName << "(v));\n";
    cpp << "        switch (g.kind(v)) {\n";
    for (const auto& c : model.alternatives)
        cpp << "        case " << name << "::Kind::" << c.publicName << ": save(ar, g.value(" << tag(c) << ", v)); break;\n";
    cpp << "        }\n    }\n";
    for (const auto& f : model.fields) if (!f.transient) cpp << "    save(ar, g." << f.name << ");\n";
    cpp << "}\n\n" << load << " {\n"
        << "    " << name << " decoded{g.get_allocator()};\n"
        << R"(    const auto readSize = [&ar]() -> uint32_t {
        const auto n = ar.readNumber();
        if (!(n >= 0 && n <= UINT32_MAX) || n != static_cast<uint32_t>(n))
            throw std::runtime_error("Invalid graph archive size");
        return static_cast<uint32_t>(n);
    };
    const auto size = readSize();
    auto edges = readSize();
)"
        << "    uint32_t counts[" << count << "];\n    for (auto& n : counts) n = readSize();\n"
        << "    for (uint32_t i = 0; i < size; ++i) {\n        const auto kind = readSize();\n        const auto parent = readSize();\n"
        << "        if (kind >= " << count << " || counts[kind] == 0) throw std::runtime_error(\"Invalid graph archive kind\");\n"
        << "        --counts[kind];\n        if (parent != decoded.null_vertex()) {\n"
        << "            if (parent >= i || !edges) throw std::runtime_error(\"Invalid graph archive parent\");\n            --edges;\n        }\n"
        << "        const auto readVertex = [&](auto tag) {\n";
    if (model.nameIndex || model.keyComponentIndex) {
        cpp << "            const auto v = decoded.appendVertex(&tag, ";
        for (size_t i = 0; i < model.components.size(); ++i) cpp << "std::tuple<>{}, ";
        cpp << "std::tuple<>{}, false, parent);\n";
    } else
        cpp << "            const auto v = decoded.add_vertex(tag, " << (takesName ? "{}, " : "") << "parent);\n";
    for (size_t i = 0; i < model.components.size(); ++i) {
        const auto& c = model.components[i];
        if (model.nameIndex && i == model.nameComponentIndex) {
            cpp << "            auto& key = decoded." << c.name << "[v];\n            load(ar, key);\n"
                << "            if (!decoded.nameIndex_.emplace(key, v).second) throw std::invalid_argument(\"duplicate vertex name\");\n";
        } else if (i == model.keyComponentIndex) {
            cpp << "            auto& componentKey = decoded." << c.name << "[v];\n            load(ar, componentKey);\n"
                << "            if (!decoded.componentIndex_.emplace(componentKey, v).second) throw std::invalid_argument(\"duplicate vertex key\");\n";
        } else {
            cpp << "            load(ar, decoded.get" << c.publicName << "(v));\n";
        }
    }
    cpp << "            load(ar, decoded.value(tag, v));\n        };\n        switch (kind) {\n";
    for (size_t i = 0; i != count; ++i) cpp << "        case " << i << ": readVertex(" << tag(model.alternatives[i]) << "); break;\n";
    cpp << "        }\n    }\n    if (edges) throw std::runtime_error(\"Invalid graph archive edge count\");\n"
        << "    for (auto n : counts) if (n) throw std::runtime_error(\"Invalid graph archive kind count\");\n";
    for (const auto& f : model.fields) if (!f.transient) cpp << "    load(ar, decoded." << f.name << ");\n";
    if (model.fields.empty()) cpp << "    g = std::move(decoded);\n";
    else cpp << "    g.swapStorage(decoded" << (std::ranges::any_of(model.fields, &FieldInfo::transient) ? ", false" : "") << ");\n";
    cpp << "}\n";
    if (!ts) return;

    const ArchiveScript writer{graph, output, imports, true};
    const ArchiveScript reader{graph, input, imports, false};
    const auto number = "a." + archiveMethod(writer.api, "writeNumber");
    const auto kinds = name + "Value.";
    *ts << "\nexport function save" << name << " (a: " << output.name << ", g: " << name << "): void {\n"
        << "    const counts = new Array<number>(" << count << ").fill(0);\n"
        << "    for (const v of g.v()) ++counts[g.w(v)];\n"
        << "    " << number << "(g.nv());\n    " << number << "(g.ne());\n    for (const n of counts) " << number << "(n);\n"
        << "    for (const v of g.v()) {\n        " << number << "(g.w(v));\n        " << number << "(g.getParent(v));\n";
    for (const auto& c : model.components) writer.value(*ts, c.valueType, "g.get" + c.publicName + "(v)", "        ");
    *ts << "        switch (g.w(v)) {\n";
    for (const auto& c : model.alternatives) {
        const auto kind = kinds + c.publicName;
        *ts << "        case " << kind << ":\n";
        writer.value(*ts, c.valueType, "g.value(" + kind + ", v)", "            ");
        *ts << "            break;\n";
    }
    *ts << "        }\n    }\n";
    for (const auto& f : model.fields) if (!f.transient) writer.value(*ts, f.type.type, "g." + f.name, "    ");
    *ts << "}\n\nexport function load" << name << " (a: " << input.name << ", g: " << name << "): void {\n"
        << "    const decoded = new " << name << "();\n    const size = readSize(a);\n    let edges = readSize(a);\n"
        << "    const counts = Array.from({ length: " << count << " }, () => readSize(a));\n"
        << "    for (let i = 0; i < size; ++i) {\n        const kind = readSize(a);\n        const parent = readSize(a);\n"
        << "        if (kind >= counts.length || !counts[kind]) throw new RangeError('Invalid graph archive kind');\n        --counts[kind];\n"
        << "        if (parent !== decoded.N) {\n            if (parent >= i || !edges) throw new RangeError('Invalid graph archive parent');\n            --edges;\n        }\n";
    std::string arguments;
    for (const auto& c : model.components) {
        const auto component = "component" + c.publicName;
        reader.value(*ts, c.valueType, component, "        ", true);
        arguments += ", " + component;
    }
    *ts << "        switch (kind) {\n";
    for (const auto& c : model.alternatives) {
        const auto kind = kinds + c.publicName;
        *ts << "        case " << kind << ": {\n";
        reader.value(*ts, c.valueType, "payload", "            ", true);
        *ts << "            decoded.addVertex(" << kind << ", payload" << arguments << ", parent);\n            break;\n        }\n";
    }
    *ts << "        }\n    }\n    if (edges || counts.some(n => n !== 0)) throw new RangeError('Invalid graph archive counts');\n";
    for (const auto& f : model.fields) if (!f.transient) reader.value(*ts, f.type.type, "decoded." + f.name, "    ");
    *ts << "    g[adopt" << name << "](decoded);\n}\n";
}

} // namespace cc::codegen
