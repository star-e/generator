#include "codegen/ArchiveEmitter.h"
#include "codegen/Documentation.h"
#include "codegen/GraphEmitter.h"
#include "model/TypeName.h"

#include <algorithm>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace cc::codegen {
namespace {

struct GraphSection {
    std::string_view name;
    std::ostringstream header{};
    std::ostringstream source{};
};

struct GraphSections {
    GraphSection graph{"Graph"}, incidence{"IncidenceGraph"}, bidirectional{"BidirectionalGraph"},
        adjacency{"AdjacencyGraph"}, vertexList{"VertexListGraph"}, edgeList{"EdgeListGraph"},
        mutation{"MutableGraph"}, components{"ComponentGraph"}, polymorphic{"PolymorphicGraph"},
        reference{"ReferenceGraph"}, referenceMutation{"MutableReferenceGraph"}, parent{"ParentGraph"},
        addressable{"AddressableGraph"}, uuid{"UuidGraph"};

    void write(std::ostream& header, std::ostream& source) const {
        for (const auto* section : {&graph, &incidence, &bidirectional, &adjacency, &vertexList, &edgeList,
                                   &mutation, &components, &polymorphic, &reference, &referenceMutation,
                                   &parent, &addressable, &uuid}) {
            auto body = section->header.view();
            if (body.empty()) continue;
            if (body.front() == '\n') body.remove_prefix(1);
            header << "\n    // " << section->name << '\n' << body;
            source << section->source.view();
        }
    }
};

struct GraphScope {
    const TypeGraph& graph;
    const TypeModule& module;
    const std::set<std::string>& members;
    const std::set<std::string>& api;

    std::string name(const std::string& qualified) const {
        const auto relative = cppName(graph, qualified, module);
        const auto first = relative.substr(0, relative.find("::"));
        if (members.contains(first) || api.contains(first)) return "::" + qualified;
        return relative;
    }
};

void vertexRange(std::ostream& out, const GraphScope& scope, const std::string& vertex) {
    out << "    using vertex_iterator = " << scope.name("cc::graph::VertexIterator") << "<" << vertex << ">;\n"
        << "    " << scope.name("cc::graph::VertexRange") << "<" << vertex << "> vertices() const noexcept { return {vertex_iterator(num_vertices())}; }\n";
}

void componentAccessors(std::ostream& out, const GraphScope& scope, const GraphType& model, const std::string& vertex,
                        const std::vector<std::string>& storage, const std::vector<std::string>& types,
                        std::string_view vectorType) {
    const auto& graph = scope.graph;
    for (size_t i = 0; i < model.components.size(); ++i) {
        const auto& tag = graph.node(model.components[i].tag);
        const auto accessor = "get" + model.components[i].publicName;
        const auto valueType = canonicalType(graph, {model.components[i].valueType, {}});
        for (const std::string qualifier : {"", " const"}) {
            if (qualifier.empty() && ((model.nameIndex && i == model.nameComponentIndex) || i == model.keyComponentIndex)) continue;
            const auto result = graph.node(valueType).qualifiedName == "bool"
                ? std::string(vectorType) + "<bool>::" + std::string(qualifier.empty() ? "reference" : "const_reference")
                : types[i] + qualifier + "&";
            out << "\n    " << result << " " << accessor << "(" << vertex << " v)" << qualifier
                << " noexcept { return this->" << storage[i] << "[v]; }\n"
                << "    " << result << " get(const " << scope.name(tag.qualifiedName) << "&, " << vertex << " v)"
                << qualifier << " noexcept { return " << accessor << "(v); }\n";
        }
    }
}

void hierarchyInterface(GraphSections& sections, const GraphScope& scope, const std::string& vertex, const std::string& rows, bool edgeList) {
    sections.incidence.header << "    using edge_descriptor = " << scope.name("cc::graph::EdgeDescriptor") << "<" << vertex << ">;\n"
        << "    using TreeEdge = " << scope.name("cc::graph::TreeEdge") << "<" << vertex << ">;\n"
        << "    using out_edge_iterator = " << scope.name("cc::graph::ChildIterator") << "<edge_descriptor, false>;\n"
        << "    " << vertex << " source(edge_descriptor e) const noexcept { return e.source; }\n"
        << "    " << vertex << " target(edge_descriptor e) const noexcept { return e.target; }\n"
        << "    " << scope.name("cc::graph::IteratorRange") << "<out_edge_iterator> out_edges(" << vertex << " v) const noexcept { return child_links(v); }\n"
        << "    " << vertex << " out_degree(" << vertex << " v) const noexcept { return num_children(v); }\n"
        << "    std::pair<edge_descriptor, bool> edge(" << vertex << " u, " << vertex << " v) const noexcept {\n"
        << "        if (u != null_vertex() && static_cast<std::uintmax_t>(v) < this->" << rows
        << ".size() && get_parent(v) == u) return {{u, v}, true};\n"
        << "        return {{null_vertex(), null_vertex()}, false};\n    }\n";

    sections.bidirectional.header << "    using in_edge_iterator = " << scope.name("cc::graph::ParentLinkIterator") << "<edge_descriptor>;\n"
        << "    " << scope.name("cc::graph::IteratorRange") << "<in_edge_iterator> in_edges(" << vertex << " v) const noexcept { return parent_links(v); }\n"
        << "    " << vertex << " in_degree(" << vertex << " v) const noexcept { return num_parents(v); }\n"
        << "    " << vertex << " degree(" << vertex << " v) const noexcept { return in_degree(v) + out_degree(v); }\n";
    sections.adjacency.header << "    using adjacency_iterator = " << scope.name("cc::graph::ChildIterator") << "<edge_descriptor, true>;\n"
        << "    " << scope.name("cc::graph::IteratorRange") << "<adjacency_iterator> adjacent_vertices(" << vertex << " v) const noexcept {\n"
        << "        const auto* rows = " << rows << ".data();\n"
        << "        return {{rows, rows[v].firstChild}, {rows, null_vertex()}};\n    }\n";
    if (edgeList) {
        sections.edgeList.header << "    " << vertex << " num_edges() const noexcept {\n        " << vertex << " count{};\n"
            << "        for (const auto& row : this->" << rows << ") count += row.childCount;\n        return count;\n    }\n"
            << "    using edge_iterator = " << scope.name("cc::graph::EdgeIterator") << "<edge_descriptor, ccstd::pmr::vector<TreeEdge>>;\n"
            << "    " << scope.name("cc::graph::IteratorRange") << "<edge_iterator> edges() const noexcept {\n"
            << "        return {{" << rows << ", 0}, {" << rows << ", static_cast<" << vertex << ">(" << rows << ".size())}};\n    }\n";
    }

    sections.reference.header << "    using ownership_descriptor = edge_descriptor;\n"
        << "    using children_iterator = out_edge_iterator;\n"
        << "    using parent_iterator = in_edge_iterator;\n";
    if (edgeList) sections.reference.header << "    using ownership_iterator = edge_iterator;\n";
    sections.reference.header << "    " << vertex << " child(ownership_descriptor link) const noexcept { return link.target; }\n"
        << "    " << vertex << " get_parent(" << vertex << " v) const noexcept { return v == null_vertex() ? v : this->"
        << rows << "[v].parent; }\n"
        << "    " << vertex << " num_children(" << vertex << " v) const noexcept { return this->" << rows << "[v].childCount; }\n"
        << "    " << scope.name("cc::graph::IteratorRange") << "<children_iterator> child_links(" << vertex << " v) const noexcept {\n"
        << "        const auto* rows = " << rows << ".data();\n"
        << "        return {{rows, rows[v].firstChild}, {rows, null_vertex()}};\n    }\n"
        << "    " << vertex << " parent(ownership_descriptor e) const noexcept { return e.source; }\n"
        << "    " << vertex << " num_parents(" << vertex << " v) const noexcept { return get_parent(v) != null_vertex(); }\n"
        << "    " << scope.name("cc::graph::IteratorRange") << "<parent_iterator> parent_links(" << vertex << " v) const noexcept { return {{{get_parent(v), v}}, {{null_vertex(), v}}}; }\n"
        << "    bool has_link(" << vertex << " u, " << vertex << " v) const noexcept { return edge(u, v).second; }\n";
}

void directedEdges(GraphSections& sections, const GraphScope& scope, const std::string& name, const std::string& vertex,
                   const std::string& outgoing, const std::string& incoming, const GraphType& model, bool references) {
    const std::string descriptor = references ? "ownership_descriptor" : "edge_descriptor";
    const std::string operation = references ? "reference" : "edge";
    auto& traversal = references ? sections.reference.header : sections.incidence.header;
    traversal << "    using " << descriptor << " = " << scope.name("cc::graph::EdgeDescriptor") << "<" << vertex << ">;\n"
        << "    " << vertex << (references ? " parent(" : " source(") << descriptor << " e) const noexcept { return e.source; }\n"
        << "    " << vertex << (references ? " child(" : " target(") << descriptor << " e) const noexcept { return e.target; }\n";
    const auto incident = [&](std::ostream& out, const char* range, const char* count, const std::string& rows, bool reversed) {
        const auto* iterator = references ? (reversed ? "parent_iterator" : "children_iterator")
                                          : (reversed ? "in_edge_iterator" : "out_edge_iterator");
        out << "    using " << iterator << " = " << scope.name("cc::detail::DirectedEdgeRange") << "<" << (reversed ? "true" : "false")
            << ", ccstd::pmr::deque<ccstd::pmr::vector<" << vertex << ">>>::Iterator;\n"
            << "    auto " << range << "(" << vertex << " v) const noexcept {\n"
            << "        return " << scope.name("cc::detail::DirectedEdgeRange") << "<" << (reversed ? "true" : "false") << ", decltype(" << rows
            << ")>{" << rows << ", v, static_cast<std::size_t>(v) + 1};\n    }\n"
            << "    std::size_t " << count << "(" << vertex << " v) const noexcept { return " << rows << "[v].size(); }\n";
    };
    incident(traversal, references ? "child_links" : "out_edges", references ? "num_children" : "out_degree", outgoing, false);
    if (!incoming.empty()) {
        auto& in = references ? sections.reference.header : sections.bidirectional.header;
        incident(in, references ? "parent_links" : "in_edges", references ? "num_parents" : "in_degree", incoming, true);
        if (references)
            in << "    " << vertex << " get_parent(" << vertex << " v) const noexcept {\n"
                << "        return v >= " << incoming << ".size() || " << incoming << "[v].empty() ? null_vertex() : " << incoming << "[v].front();\n    }\n";
        else in << "    std::size_t degree(" << vertex << " v) const noexcept { return in_degree(v) + out_degree(v); }\n";
    }
    if (!references)
        sections.adjacency.header << "    using adjacency_iterator = ccstd::pmr::vector<" << vertex << ">::const_iterator;\n"
            << "    const auto& adjacent_vertices(" << vertex << " v) const noexcept { return " << outgoing << "[v]; }\n";
    if (model.edgeList && references) traversal << "    using ownership_iterator = children_iterator;\n";
    if (model.edgeList && !references) {
        sections.edgeList.header << "    using edge_iterator = out_edge_iterator;\n"
            << "    auto edges() const noexcept { return " << scope.name("cc::detail::DirectedEdgeRange") << "<false, decltype(" << outgoing
            << ")>{" << outgoing << ", 0, " << outgoing << ".size()}; }\n"
            << "    std::size_t num_edges() const noexcept {\n        std::size_t count{};\n"
            << "        for (const auto& row : " << outgoing << ") count += row.size();\n        return count;\n    }\n";
    }
    traversal << "    std::pair<" << descriptor << ", bool> " << operation << "(" << vertex << " u, " << vertex << " v) const noexcept {\n"
        << "        if (u < " << outgoing << ".size()) {\n            const auto& row = " << outgoing << "[u];\n"
        << "            if (std::find(row.begin(), row.end(), v) != row.end()) return {{u, v}, true};\n        }\n"
        << "        return {{null_vertex(), null_vertex()}, false};\n    }\n";
    if (references) traversal << "    bool has_link(" << vertex << " u, " << vertex << " v) const noexcept { return reference(u, v).second; }\n";
    if (!model.mutableGraph) return;
    auto& mutations = references ? sections.referenceMutation : sections.mutation;
    mutations.header << "    std::pair<" << descriptor << ", bool> add_" << operation << "(" << vertex << " u, " << vertex << " v);\n"
        << "    void remove_" << operation << "(" << descriptor << " e) noexcept;\n"
        << "    void remove_" << operation << "(" << vertex << " u, " << vertex << " v) noexcept;\n";
    auto& source = mutations.source;
    source << "\nstd::pair<" << name << "::" << descriptor << ", bool> " << name << "::add_" << operation << "(" << vertex << " u, " << vertex << " v) {\n"
           << "    if (u >= " << outgoing << ".size() || v >= " << outgoing << ".size()) return {{null_vertex(), null_vertex()}, false};\n"
           << "    " << outgoing << "[u].push_back(v);\n";
    if (!incoming.empty())
        source << "    try {\n        " << incoming << "[v].push_back(u);\n    } catch (...) {\n"
               << "        " << outgoing << "[u].pop_back();\n        throw;\n    }\n";
    source << "    return {{u, v}, true};\n}\n"
           << "\nvoid " << name << "::remove_" << operation << "(" << descriptor << " e) noexcept {\n"
           << "    if (e.source >= " << outgoing << ".size()) return;\n    auto& out = " << outgoing << "[e.source];\n"
           << "    const auto it = std::find(out.begin(), out.end(), e.target);\n    if (it == out.end()) return;\n    out.erase(it);\n";
    if (!incoming.empty())
        source << "    auto& in = " << incoming << "[e.target];\n    in.erase(std::find(in.begin(), in.end(), e.source));\n";
    source << "}\n\nvoid " << name << "::remove_" << operation << "(" << vertex << " u, " << vertex << " v) noexcept {\n"
           << "    if (u >= " << outgoing << ".size() || v >= " << outgoing << ".size()) return;\n"
           << "    auto& out = " << outgoing << "[u];\n    out.erase(std::remove(out.begin(), out.end(), v), out.end());\n";
    if (!incoming.empty())
        source << "    auto& in = " << incoming << "[v];\n    in.erase(std::remove(in.begin(), in.end(), u), in.end());\n";
    source << "}\n";
}

void parentLookup(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
                  const std::string& member, const std::string& accessor) {
    out << "    " << vertex << " locate_child(" << vertex << " parent, std::string_view name) const noexcept;\n";
    source << "\n" << name << "::" << vertex << " " << name << "::locate_child(" << vertex
           << " parent, std::string_view name) const noexcept {\n"
           << "    const auto count = this->" << member << ".size();\n"
           << "    if (parent == null_vertex()) {\n"
           << "        for (std::size_t i = 0; i < count; ++i) {\n"
           << "            const auto v = static_cast<" << vertex << ">(i);\n"
           << "            if (get_parent(v) == null_vertex() && std::string_view{this->" << accessor << "(v)} == name) return v;\n"
           << "        }\n"
           << "    } else if (static_cast<std::uintmax_t>(parent) < count) {\n"
           << "        for (const auto link : child_links(parent)) {\n"
           << "            const auto v = child(link);\n"
           << "            if (std::string_view{this->" << accessor << "(v)} == name) return v;\n"
           << "        }\n"
           << "    }\n    return null_vertex();\n}\n";
}

void hierarchyMutation(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
                       const std::string& rows, bool serializable) {
    out << "    std::pair<edge_descriptor, bool> add_edge(" << vertex << " u, " << vertex << " v) noexcept;\n"
        << "    void remove_edge(edge_descriptor e) noexcept;\n";
    source << "\nstd::pair<" << name << "::edge_descriptor, bool> " << name << "::add_edge(" << vertex << " u, " << vertex << " v) noexcept {\n"
           << "    if (static_cast<std::uintmax_t>(u) >= " << rows << ".size() || static_cast<std::uintmax_t>(v) >= " << rows << ".size() ||\n"
           << "        " << rows << "[v].parent != null_vertex()" << (serializable ? " || u >= v" : "") << ") return {{null_vertex(), null_vertex()}, false};\n";
    if (!serializable)
        source << "    for (auto p = u; p != null_vertex(); p = " << rows << "[p].parent)\n"
               << "        if (p == v) return {{null_vertex(), null_vertex()}, false};\n";
    source << "    auto& parent = " << rows << "[u];\n"
           << "    auto& child = " << rows << "[v];\n"
           << "    auto* slot = &parent.firstChild;\n"
           << "    while (*slot != null_vertex() && *slot < v) slot = &" << rows << "[*slot].nextSibling;\n"
           << R"(    child.nextSibling = *slot;
    if (*slot == null_vertex()) parent.lastChild = v;
    *slot = v;
    child.parent = u;
    ++parent.childCount;
    return {{u, v}, true};
}
)";
    source << "\nvoid " << name << "::remove_edge(edge_descriptor e) noexcept {\n"
           << "    if (!edge(e.source, e.target).second) return;\n"
           << "    auto& parent = " << rows << "[e.source];\n"
           << "    auto& child = " << rows << "[e.target];\n"
           << "    auto previous = null_vertex();\n"
           << "    auto* slot = &parent.firstChild;\n"
           << "    while (*slot != e.target) {\n"
           << "        previous = *slot;\n"
           << "        slot = &" << rows << "[previous].nextSibling;\n"
           << R"(    }
    *slot = child.nextSibling;
    if (parent.lastChild == e.target) parent.lastChild = previous;
    child.parent = child.nextSibling = null_vertex();
    --parent.childCount;
}
)";
}

void addressableGraph(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
                      const std::string& member, const std::string& accessor) {
    out << "\n    " << vertex << " locate(std::string_view refPath, " << vertex << " start = null_vertex()) const noexcept;\n"
        << "    bool exists(std::string_view refPath, " << vertex << " start = null_vertex()) const noexcept {\n"
        << "        return locate(refPath, start) != null_vertex();\n    }\n"
        << "    ccstd::pmr::string path(" << vertex << " v) const;\n";
    source << "\n" << name << "::" << vertex << " " << name << "::locate(std::string_view refPath, " << vertex << " start) const noexcept {\n"
           << "    if (refPath.empty()) return null_vertex();\n"
           << "    if (refPath.front() == '/') start = null_vertex();\n"
           << "    else if (start != null_vertex() && static_cast<std::uintmax_t>(start) >= this->" << member << ".size()) return null_vertex();\n"
           << R"(
    while (!refPath.empty()) {
        const auto slash = refPath.find('/');
        const auto segment = refPath.substr(0, slash);
        refPath.remove_prefix(slash == std::string_view::npos ? refPath.size() : slash + 1);
        if (segment.empty() || segment == ".") continue;
        if (segment == "..") {
            if (start == null_vertex()) return null_vertex();
            start = get_parent(start);
        } else {
            start = locate_child(start, segment);
            if (start == null_vertex()) return null_vertex();
        }
    }
    return start;
}
)";
    source << "\nccstd::pmr::string " << name << "::path(" << vertex << " v) const {\n"
           << "    if (v != null_vertex() && static_cast<std::uintmax_t>(v) >= this->" << member << ".size())\n"
           << "        throw std::out_of_range(\"invalid " << name << " vertex\");\n"
           << "    ccstd::pmr::string result(get_allocator());\n"
           << "    std::size_t length = 0;\n"
           << "    for (auto u = v; u != null_vertex(); u = get_parent(u)) {\n"
           << "        const auto size = this->" << accessor << "(u).size();\n"
           << "        if (size >= result.max_size() - length) throw std::length_error(\"graph path too long\");\n"
           << "        length += size + 1;\n    }\n"
           << "    result.resize(length);\n"
           << "    for (auto u = v; u != null_vertex(); u = get_parent(u)) {\n"
           << "        const auto& segment = this->" << accessor << "(u);\n"
           << "        length -= segment.size();\n"
           << "        segment.copy(result.data() + length, segment.size());\n"
           << "        result[--length] = '/';\n    }\n    return result;\n}\n";
}

void graphConstructors(std::ostream& out, std::ostream& source, const std::string& name,
                       const std::vector<std::string>& storage, const std::vector<BoundField>& fields,
                       bool copyable, bool immovable) {
    const auto& first = storage.front();
    out << "    explicit " << name << "(const allocator_type& alloc)";
    for (const auto& member : storage)
        out << (member == first ? " : " : ", ") << member << "(alloc)";
    for (const auto& field : fields)
        if (field.usesAllocator) out << ", " << field.name << "(alloc)";
    out << " {}\n";
    if (!immovable) {
        out << "    " << name << "(const " << name << "& rhs, const allocator_type& alloc)" << (copyable ? ";\n" : " = delete;\n")
            << "    " << name << "(" << name << "&& rhs, const allocator_type& alloc);\n";
        for (bool move : {false, true}) {
            if (!move && !copyable) continue;
            source << "\n" << name << "::" << name << "(" << (move ? "" : "const ") << name
                   << (move ? "&&" : "&") << " rhs, const allocator_type& alloc)\n";
            const auto initialize = [&](const std::string& member, bool usesAllocator) {
                source << (member == first ? ": " : ", ") << member << "("
                       << (move ? "std::move(rhs." : "rhs.") << member << (move ? ")" : "")
                       << (usesAllocator ? ", alloc)" : ")");
            };
            for (const auto& member : storage) initialize(member, true);
            for (const auto& field : fields) initialize(field.name, field.usesAllocator);
            source << " {}\n";
        }
    }
    out << "    " << name << "(" << name << "&&)" << (immovable ? " = delete;\n" : " noexcept = default;\n")
        << "    " << name << "(const " << name << "&) = delete;\n"
        << "    " << name << "& operator=(" << name << "&&)" << (immovable ? " = delete;\n" : " noexcept = default;\n")
        << "    " << name << "& operator=(const " << name << "&)" << (copyable && !immovable ? " = default;\n" : " = delete;\n")
        << "    allocator_type get_allocator() const noexcept { return {this->" << first << ".get_allocator().resource()}; }\n";
}

bool hasMoveOnlyValue(const TypeGraph& graph, TypeId id, std::set<uint32_t>& visited) {
    if (!visited.insert(id.value).second) return false;
    const auto& node = graph.node(id);
    if (const auto* alias = std::get_if<AliasType>(&node.data)) return hasMoveOnlyValue(graph, alias->target, visited);
    const auto fieldMoves = [&](const FieldInfo& f) { return hasMoveOnlyValue(graph, f.type.type, visited); };
    if (const auto* record = std::get_if<RecordType>(&node.data))
        return record->moveOnly || std::ranges::any_of(record->fields, fieldMoves) ||
               std::ranges::any_of(record->bases, [&](TypeId base) { return hasMoveOnlyValue(graph, base, visited); });
    if (const auto* nested = std::get_if<GraphType>(&node.data)) {
        const auto columnMoves = [&](const ComponentInfo& c) { return hasMoveOnlyValue(graph, c.valueType, visited); };
        return std::ranges::any_of(nested->components, columnMoves) || std::ranges::any_of(nested->alternatives, columnMoves) ||
               std::ranges::any_of(nested->fields, fieldMoves);
    }
    if (const auto* instance = std::get_if<TemplateInstanceType>(&node.data);
        instance && std::get<ExternalTypeNode>(graph.node(instance->declaration).data).container != ContainerKind::None) {
        for (const auto& arg : instance->arguments)
            if (const auto* element = std::get_if<TypeId>(&arg); element && hasMoveOnlyValue(graph, *element, visited)) return true;
    }
    return false;
}

struct Alternative {
    std::string tag;
    std::string kind;
    std::string type;
    std::string member;
    bool isBoolean;

    std::string element(const std::string& index) const {
        return "this->" + member + "[" + index + "]" + (isBoolean ? "[0]" : "");
    }
};

std::string templateParam(std::string param, const std::string& graphName, const std::string& vertex,
                          const std::vector<Alternative>& alternatives) {
    while (param == vertex || param == graphName || std::ranges::any_of(alternatives, [&](const auto& alt) {
        return alt.tag == param || alt.tag.starts_with(param + "::");
    })) param += "T";
    return param;
}

template<class Body>
void tagBranches(std::ostream& out, const std::vector<Alternative>& alternatives, const std::string& tag,
                 const std::string& indent, bool constrained, Body writeBranch) {
    const auto bodyIndent = alternatives.size() > 1 ? indent + "    " : indent;
    for (size_t i = 0; i + 1 < alternatives.size(); ++i) {
        out << (i ? "" : indent) << "if constexpr (std::is_same_v<" << tag << ", " << alternatives[i].tag << ">) {\n";
        writeBranch(alternatives[i], bodyIndent);
        out << indent << "} else ";
    }
    const bool hasElse = alternatives.size() > 1;
    if (hasElse) out << "{\n";
    if (!constrained)
        out << bodyIndent << "static_assert(std::is_same_v<" << tag << ", " << alternatives.back().tag << ">, \"unknown graph tag\");\n";
    writeBranch(alternatives.back(), bodyIndent);
    if (hasElse) out << indent << "}\n";
}

void payloadAccess(std::ostream& out, const GraphScope& scope, const std::vector<Alternative>& alternatives,
                   const std::string& vertex, const std::string& handles, const std::string& tagParam) {
    out << "\n    enum class Kind { ";
    for (size_t i = 0; i < alternatives.size(); ++i) out << (i ? ", " : "") << alternatives[i].kind;
    out << " };\n    Kind kind(" << vertex << " v) const { return this->" << handles << ".at(v).kind; }\n"
        << "    template<class " << tagParam << ">\n    bool holds(" << vertex << " v) const noexcept {\n"
        << "        if (static_cast<std::uintmax_t>(v) >= this->" << handles << ".size()) return false;\n";
    tagBranches(out, alternatives, tagParam, "        ", false, [&](const auto& alt, const auto& indent) {
        out << indent << "return this->" << handles << "[v].kind == Kind::" << alt.kind << ";\n";
    });
    out << "    }\n";
    for (const auto& alt : alternatives) {
        for (const std::string qualifier : {"", " const"}) {
            out << "    " << alt.type << qualifier << "& value(const " << alt.tag << "&, " << vertex << " v)" << qualifier << " {\n"
                << "        if (!holds<" << alt.tag << ">(v)) throw std::bad_variant_access{};\n"
                << "        return " << alt.element("this->" + handles + "[v].index") << ";\n    }\n";
        }
    }
    const auto visitorType = vertex == "Visitor" ? "ObjectVisitor" : "Visitor";
    const auto othersType = vertex == "Others" ? "OtherVisitors" : "Others";
    for (const std::string qualifier : {"", " const"}) {
        out << "\n    template<class " << visitorType << ", class... " << othersType << ">\n"
            << "    decltype(auto) visit_object(" << vertex << " v, " << visitorType << "&& visitor, " << othersType << "&&... others)" << qualifier << " {\n"
            << "        if constexpr (sizeof...(others) > 0) {\n"
            << "            auto handlers = " << scope.name("cc::overloaded") << "{std::forward<" << visitorType << ">(visitor), std::forward<" << othersType << ">(others)...};\n"
            << "            return this->visit_object(v, handlers);\n"
            << "        } else {\n            const auto& handle = this->" << handles << ".at(v);\n"
            << "            switch (handle.kind) {\n";
        for (const auto& alt : alternatives)
            out << "            case Kind::" << alt.kind << ": return std::forward<" << visitorType
                << ">(visitor)(" << alt.element("handle.index") << ");\n";
        out << "            }\n            throw std::bad_variant_access{};\n        }\n    }\n";
    }
}

void removeVertex(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
                  const GraphType& model, const std::vector<std::string>& storage,
                  const std::string& outgoing, const std::string& incoming) {
    out << "    void remove_vertex(" << vertex << " v);\n";
    source << "\nvoid " << name << "::remove_vertex(" << vertex << " v) {\n"
           << "    if (v >= this->" << storage.front() << ".size()) throw std::out_of_range(\"invalid vertex\");\n";
    const auto eraseKey = [&](const std::string& map, const std::string& column) {
        source << "    this->" << map << ".erase(this->" << column << "[v]);\n"
               << "    for (auto& entry : this->" << map << ") if (entry.second > v) --entry.second;\n";
    };
    if (model.keyComponentIndex) eraseKey("componentIndex_", storage.at(*model.keyComponentIndex));
    if (model.nameIndex) eraseKey("nameIndex_", storage.at(*model.nameComponentIndex));
    for (const auto& c : model.components) {
        source << "    static_assert(std::is_nothrow_move_assignable_v<decltype(" << c.name << ")::value_type>);\n"
               << "    this->" << c.name << ".erase(this->" << c.name << ".begin() + v);\n";
    }
    if (!outgoing.empty()) {
        source << R"(    const auto compact = [v](auto& rows) {
        for (std::size_t i = v; i + 1 < rows.size(); ++i) rows[i].swap(rows[i + 1]);
        rows.pop_back();
        for (auto& row : rows) {
            row.erase(std::remove(row.begin(), row.end(), v), row.end());
            for (auto& neighbor : row) if (neighbor > v) --neighbor;
        }
    };
)" << "    compact(this->" << outgoing << ");\n";
        if (!incoming.empty()) source << "    compact(this->" << incoming << ");\n";
    }
    source << "}\n";
}

void addVertex(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
               const std::vector<ComponentInfo>& components, const std::string& input,
               const std::vector<Alternative>& alternatives, bool hierarchy,
               const std::string& keyColumn, const std::string& keyType) {
    std::string value;
    if (!keyColumn.empty()) {
        value = keyType + " key";
    }
    if (!input.empty()) {
        value += value.empty() ? "std::string_view value" : ", std::string_view value";
    }
    const std::string definition = "\n" + name + "::" + vertex + " " + name + "::add_vertex(";
    const auto overload = [&](const std::string& tag) {
        std::string parameters = tag.empty() ? "" : "const " + tag + "& tag";
        if (!value.empty()) parameters += (parameters.empty() ? "" : ", ") + value;
        out << "    " << vertex << " add_vertex(" << parameters;
        if (!input.empty()) out << " = {}";
        if (hierarchy) {
            const auto parent = (parameters.empty() ? "" : ", ") + vertex + " parent";
            if (tag.empty()) {
                out << ") { return add_vertex(";
                if (!keyColumn.empty()) out << "key, ";
                if (!input.empty()) out << "value, ";
                out << "null_vertex()); }\n    " << vertex << " add_vertex(" << parameters << parent;
            } else {
                out << parent << " = null_vertex()";
            }
            parameters += parent;
        }
        out << ");\n";
        source << definition << parameters << ") {\n    return add_vertex(";
        if (!tag.empty()) source << "tag, ";
        source << "std::piecewise_construct";
        if (!tag.empty()) source << ", std::tuple<>{}";
        for (const auto& component : components)
            source << ", " << (component.name == input ? "std::forward_as_tuple(value)" :
                               component.name == keyColumn ? "std::forward_as_tuple(key)" : "std::tuple<>{}");
        if (hierarchy) source << ", parent";
        source << ");\n}\n";
    };
    if (alternatives.empty()) overload({});
    for (const auto& alt : alternatives) overload(alt.tag);
}

void piecewiseAddVertex(std::ostream& declaration, std::ostream& out, const std::string& name, const std::string& vertex,
                   const std::vector<ComponentInfo>& components, const std::vector<std::string>& storage,
                   const std::string& rows, const std::vector<Alternative>& alternatives,
                   const std::string& handles, const GraphType& model, const std::string& children, const std::string& tagType) {
    const bool indexedNames = model.nameIndex;
    const bool tagged = !alternatives.empty();
    const bool hierarchy = !rows.empty() || !children.empty();
    std::string templateParams = tagged ? "class " + tagType : "";
    std::string tupleParams, tupleTypes;
    const auto append = [](std::string& list, const std::string& item) {
        if (!list.empty()) list += ", ";
        list += item;
    };
    const auto typeParam = [&](std::string type) { return templateParam(std::move(type), name, vertex, alternatives); };
    std::vector<std::string> componentArgs;
    const auto tupleParam = [&](const std::string& type, const std::string& arg) {
        append(templateParams, "class " + type);
        append(tupleParams, type + "&& " + arg);
        if (!tupleTypes.empty()) tupleTypes += ",\n                                 ";
        tupleTypes += "typename std::tuple_size<std::decay_t<" + type + ">>::type";
        return "std::forward<" + type + ">(" + arg + ")";
    };
    const auto payloadArgs = tagged ? tupleParam(typeParam("PayloadArgs"), "payloadArgs") : "";
    for (size_t i = 0; i < components.size(); ++i)
        componentArgs.push_back(tupleParam(typeParam("Args" + std::to_string(i)), "args" + components[i].publicName));
    const auto templateHead = [&](std::ostream& stream, std::string_view indent) {
        stream << '\n' << indent << "template<" << templateParams;
        if (!tagged) return;
        stream << ",\n" << indent << "         std::enable_if_t<";
        for (size_t i = 0; i < alternatives.size(); ++i) {
            if (i) stream << " ||\n" << indent << "                          ";
            stream << "std::is_same_v<" << tagType << ", " << alternatives[i].tag << ">";
        }
        stream << ", int>";
    };
    std::string params = tagged ? "const " + tagType + "&, " : "";
    params += "std::piecewise_construct_t, " + tupleParams;
    if (hierarchy) params += ", " + vertex + " parent";
    templateHead(declaration, "    ");
    if (tagged) declaration << " = 0";
    declaration << ",\n             class = std::void_t<" << tupleTypes << ">>\n    " << vertex << " add_vertex("
                << params << (hierarchy ? " = null_vertex()" : "") << ");\n";
    templateHead(out, "");
    out << ", class>\n" << name << "::" << vertex << " " << name << "::add_vertex(" << params << ") {\n";
    const auto count = "this->" + storage.front() + ".size()";
    const auto descriptor = name + "::" + vertex;
    if (hierarchy)
        out << "    if (parent != null_vertex() && static_cast<std::uintmax_t>(parent) >= " << count << ")\n"
             << "        throw std::out_of_range(\"invalid " << name << " parent\");\n";
    out << "    if (" << count << " >= (std::numeric_limits<" << descriptor << ">::max)())\n"
         << "        throw std::length_error(\"" << name << " vertex limit exceeded\");\n"
         << "    const auto vertex = static_cast<" << descriptor << ">(" << count << ");\n";
    if (tagged && model.keyComponentIndex) out << "    bool indexedKey = false;\n";
    if (indexedNames) out << "    auto entry = this->nameIndex_.end();\n";
    if (!rows.empty()) out << "    auto& edges = this->" << rows << ";\n";
    if (!children.empty()) out << "    bool linked = false;\n";
    out << "    try {\n";
    const auto emplaceTuple = [&](const std::string& column, const std::string& tuple, std::string_view indent) {
        out << indent << "std::apply([&](auto&&... args) {\n"
             << indent << "    " << column << ".emplace_back(std::forward<decltype(args)>(args)...);\n"
             << indent << "}, " << tuple << ");\n";
    };
    const auto emplaceComponent = [&](size_t i) {
        emplaceTuple("this->" + storage[i], componentArgs[i], "        ");
    };
    if (indexedNames) {
        const auto i = *model.nameComponentIndex;
        emplaceComponent(i);
        out << "        const auto inserted = this->nameIndex_.emplace(this->" << storage[i] << ".back(), vertex);\n"
            << "        if (!inserted.second) throw std::invalid_argument(\"duplicate vertex name\");\n"
            << "        entry = inserted.first;\n";
    }
    for (size_t i = 0; i < components.size(); ++i)
        if (!indexedNames || i != *model.nameComponentIndex) emplaceComponent(i);
    for (size_t i = components.size(); i < storage.size(); ++i) {
        const auto& field = storage[i];
        if (field != rows && field != handles)
            out << "        this->" << field << ".emplace_back();\n";
    }
    if (!rows.empty()) out << "        edges.push_back(TreeEdge{parent});\n";
    if (!children.empty()) out << "        if (parent != null_vertex()) linked = add_reference(parent, vertex).second;\n";
    if (model.keyComponentIndex) {
        out << "        if (!this->componentIndex_.emplace(this->" << storage[*model.keyComponentIndex] << ".back(), vertex).second)\n"
             << "            throw std::invalid_argument(\"duplicate vertex key\");\n";
        if (tagged) out << "        indexedKey = true;\n";
    }
    // Append the payload last so rollback only needs the common per-vertex storage and indices.
    if (tagged) {
        tagBranches(out, alternatives, tagType, "        ", true, [&](const auto& alt, const auto& indent) {
            const auto column = "this->" + alt.member;
            out << indent << "this->" << handles << ".push_back({Kind::" << alt.kind << ", static_cast<" << descriptor
                << ">(" << column << ".size())});\n";
            if (alt.isBoolean)
                out << indent << column << ".push_back({std::make_from_tuple<bool>(" << payloadArgs << ")});\n";
            else emplaceTuple(column, payloadArgs, indent);
        });
    }
    out << "    } catch (...) {\n";
    if (indexedNames) out << "        if (entry != this->nameIndex_.end()) this->nameIndex_.erase(entry);\n";
    if (!children.empty()) out << "        if (linked) this->" << children << "[parent].pop_back();\n";
    if (tagged && model.keyComponentIndex)
        out << "        if (indexedKey) this->componentIndex_.erase(this->" << storage[*model.keyComponentIndex] << ".back());\n";
    for (const auto& field : storage)
        out << "        if (this->" << field << ".size() > vertex) this->" << field << ".pop_back();\n";
    out << "        throw;\n    }\n";
    if (!rows.empty()) {
        out << R"(    if (parent != null_vertex()) {
        auto& parentLinks = edges[parent];
        if (parentLinks.lastChild == null_vertex()) {
            parentLinks.firstChild = vertex;
        } else {
            edges[parentLinks.lastChild].nextSibling = vertex;
        }
        parentLinks.lastChild = vertex;
        ++parentLinks.childCount;
    }
)";
    }
    out << "    return vertex;\n}\n";
}

} // namespace

bool hasMoveOnlyValue(const TypeGraph& graph, TypeId id) {
    std::set<uint32_t> visited;
    return hasMoveOnlyValue(graph, id, visited);
}

std::string emitCppGraph(const TypeGraph& graph, TypeId id, std::ostream& out, std::ostream& templateDefinitions, std::ostream& source,
                         std::set<std::string>& headers, std::set<const TypeNode*>& forwards,
                         const std::vector<BoundField>& fields, const std::string& methods, const TypeModule* archive,
                         bool transactionalLoad) {
    const auto& node = graph.node(id);
    const auto& model = std::get<GraphType>(node.data);
    if (!model.indexedVertices || (model.components.empty() && model.alternatives.empty()))
        throw std::runtime_error(node.qualifiedName + ": C++ graph output requires indexed storage and at least one component or alternative");
    // The descriptor alias is a scoped declaration. Its resolved integer type is stored on the graph.
    const auto children = graph.out_edges(id.value);
    const auto alias = std::ranges::find_if(children, [&](auto edge) {
        return std::holds_alternative<AliasType>(graph.node(TypeId{graph.target(edge)}).data);
    });
    if (alias == children.end()) throw std::runtime_error(node.qualifiedName + ": missing vertex descriptor alias");
    const auto& vertex = graph.node(TypeId{graph.target(*alias)}).name;
    const auto& integer = graph.node(model.vertexType);
    const auto& name = node.name;
    // Vertex storage also uses the native PMR vector when there are no explicit vector fields.
    const auto storageVector = graph.findType("ccstd::pmr::vector");
    if (model.indexedStorage) headers.insert("vector");
    else if (storageVector != TypeId{} && !graph.node(storageVector).cppHeader.empty()) headers.insert(graph.node(storageVector).cppHeader);
    else headers.insert("cocos/base/std/container/vector.h");
    headers.insert("utility");
    if (!integer.cppHeader.empty()) headers.insert(integer.cppHeader);
    bool copyable = true;
    const auto columnValue = [&](TypeId type) -> const TypeNode& {
        while (const auto* aliasType = std::get_if<AliasType>(&graph.node(type).data)) type = aliasType->target;
        return graph.node(type);
    };
    std::vector<std::string> storage;
    std::set<std::string> members{vertex, name};
    for (const auto& component : model.components) {
        storage.push_back(component.name);
        members.insert(component.name);
    }
    for (const auto& alt : model.alternatives) members.insert(alt.name);
    std::set<std::string> api{"allocator_type", "get_allocator"};
    if (!model.alternatives.empty()) {
        for (const auto* symbol : {"Kind", "kind", "holds", "value", "visit_object", "VertexHandle"}) {
            if (members.contains(symbol))
                throw std::runtime_error(node.qualifiedName + ": polymorphic graph member name collision: " + symbol);
            api.insert(symbol);
        }
    }
    if (model.keyComponentIndex) {
        for (const auto* symbol : {"componentIndex_", "contains", "vertex", "find", "null_vertex", "reserve"}) {
            if (members.contains(symbol)) throw std::runtime_error(node.qualifiedName + ": component index collides with " + symbol);
            api.insert(symbol);
        }
    }
    std::string nameSetter;
    if (model.nameIndex) {
        const auto& component = model.components.at(*model.nameComponentIndex);
        nameSetter = "set" + component.publicName;
        std::set<std::string> symbols{"nameIndex_", "contains", "vertex", "find", "null_vertex"};
        if (model.mutableGraph) symbols.insert(nameSetter);
        for (const auto& symbol : symbols) {
            if (members.contains(symbol))
                throw std::runtime_error(node.qualifiedName + ": name index collides with C++ member " + symbol);
        }
        api.insert(symbols.begin(), symbols.end());
    }
    if (model.vertexList) {
        if (members.contains("vertex_iterator"))
            throw std::runtime_error(node.qualifiedName + ": vertex list API collides with vertex_iterator");
        api.insert({"vertices", "num_vertices", "vertex_iterator"});
    }
    if (model.edgeList) {
        std::set<std::string> symbols{"edges", "num_edges", "edge_iterator"};
        if (model.hierarchy) symbols.insert("ownership_iterator");
        for (const auto& symbol : symbols) {
            if (members.contains(symbol)) throw std::runtime_error(node.qualifiedName + ": edge list API collides with " + symbol);
        }
        api.insert(symbols.begin(), symbols.end());
    }
    if (model.indexedStorage) api.insert("reserve");
    if (model.incidence) {
        std::set<std::string> symbols{"edge_descriptor", "out_edge_iterator", "adjacency_iterator", "null_vertex", "source", "target", "out_edges", "out_degree",
                                      "adjacent_vertices", "edge"};
        if (model.bidirectional) symbols.insert({"in_edge_iterator", "in_edges", "in_degree", "degree"});
        if (model.mutableGraph) symbols.insert({"add_edge", "remove_edge"});
        for (const auto& symbol : symbols) {
            if (members.contains(symbol)) throw std::runtime_error(node.qualifiedName + ": edge API collides with " + symbol);
        }
        api.insert(symbols.begin(), symbols.end());
    }
    if (model.mutableGraph) {
        std::set<std::string> symbols{"add_vertex"};
        if (model.removeVertex) symbols.insert("remove_vertex");
        for (const auto& symbol : symbols)
            if (members.contains(symbol)) throw std::runtime_error(node.qualifiedName + ": vertex API collides with " + symbol);
        api.insert(symbols.begin(), symbols.end());
        if (model.hierarchy && !model.separateEdges) api.insert({"add_edge", "remove_edge"});
    }
    if (model.separateEdges) {
        std::set<std::string> symbols{"null_vertex", "ownership_descriptor", "parent_iterator", "children_iterator", "get_parent", "num_children", "child_links", "child", "parent",
                                      "parent_links", "num_parents", "reference", "has_link"};
        if (model.mutableGraph) symbols.insert({"add_reference", "remove_reference"});
        for (const auto& symbol : symbols)
            if (members.contains(symbol)) throw std::runtime_error(node.qualifiedName + ": reference API collides with " + symbol);
        api.insert(symbols.begin(), symbols.end());
    } else if (model.hierarchy) api.insert({"null_vertex", "TreeEdge", "ownership_descriptor", "out_edge_iterator", "in_edge_iterator", "adjacency_iterator",
        "children_iterator", "parent_iterator", "get_parent", "num_children", "child_links", "child", "parent", "parent_links",
        "num_parents", "edge_descriptor", "source", "target", "out_edges", "out_degree", "in_edges", "in_degree", "degree",
        "adjacent_vertices", "edge", "has_link"});
    if (model.parentGraph) api.insert("locate_child");
    if (model.addressableGraph) api.insert({"locate", "path", "exists"});
    for (const auto& c : model.components) {
        api.insert("get");
        api.insert("get" + c.publicName);
    }
    for (const auto& f : model.fields) {
        if (hasMoveOnlyValue(graph, f.type.type)) copyable = false;
        if (api.contains(f.name) || !members.insert(f.name).second)
            throw std::runtime_error(node.qualifiedName + ": graph field collides with C++ member " + f.name);
    }
    for (const auto& method : model.methods) {
        if (api.contains(method.name) || method.name == name)
            throw std::runtime_error(method.result.location.describe() + ": graph method collides with C++ member " + method.name);
        members.insert(method.name);
    }
    const GraphScope scope{graph, graph.modules.at(node.module.value), members, api};
    const auto columnType = [&](const ComponentInfo& component) {
        if (hasMoveOnlyValue(graph, component.valueType)) copyable = false;
        const auto& tag = graph.node(component.tag);
        if (tag.module != node.module && !tag.cppHeader.empty()) {
            headers.insert(tag.cppHeader);
        } else if (component.tag.value > id.value || tag.module != node.module) {
            forwards.insert(&tag);
        }
        return formatTypeName(graph, component.valueType, [&](const TypeNode& type) {
            collectCppHeader(graph, scope.module, type, headers);
            return scope.name(type.qualifiedName);
        });
    };
    std::vector<std::string> types;
    for (const auto& component : model.components) types.push_back(columnType(component));
    const auto keyColumn = model.keyComponentIndex ? storage.at(*model.keyComponentIndex) : "";
    const auto keyType = model.keyComponentIndex ? types.at(*model.keyComponentIndex) : "";
    std::vector<Alternative> alternatives;
    for (const auto& alt : model.alternatives) {
        const auto& tag = graph.node(alt.tag);
        const auto& payload = columnValue(alt.valueType);
        if (payload.qualifiedName == "void" || std::holds_alternative<InterfaceType>(payload.data))
            throw std::runtime_error(node.qualifiedName + "." + alt.name + ": polymorphic storage requires an object type");
        const bool isBoolean = payload.qualifiedName == "bool";
        alternatives.push_back({scope.name(tag.qualifiedName), alt.publicName, columnType(alt), alt.name, isBoolean});
    }
    bool currentPrivate = false;
    const auto writeDeclaredMethods = [&] {
        if (methods.empty()) return;
        out << '\n' << methods;
        currentPrivate = model.methods.back().cppPrivate;
        memberAccess(out, currentPrivate, false);
    };
    const auto writeFields = [&] {
        for (size_t i = 0; i < fields.size(); ++i) {
            const auto& field = fields[i];
            memberAccess(out, currentPrivate, model.fields[i].cppPrivate);
            writeDocumentation(out, field.documentation, "    ");
            out << "    " << field.cpp << " " << field.name;
            if (!field.cppDefault.empty()) out << "{" << field.cppDefault << "}";
            out << ";\n";
        }
    };
    if (model.indexedStorage) {
        const auto base = formatTypeName(graph, *model.indexedStorage, [&](const TypeNode& part) {
            collectCppHeader(graph, scope.module, part, headers);
            return scope.name(part.qualifiedName);
        });
        out << '\n';
        writeDocumentation(out, node.documentation);
        out << "struct " << name << " : public " << base << " {\n"
            << "    using " << vertex << " = " << scope.name(integer.qualifiedName) << ";\n";
        if (model.edgeList) out << "    using " << base << "::edges;\n    using " << base << "::num_edges;\n";
        out << "    void reserve(" << vertex << " capacity) {\n        " << base << "::reserve(capacity);\n";
        for (const auto& column : storage) out << "        this->" << column << ".reserve(capacity);\n";
        out << "    }\n";
        componentAccessors(out, scope, model, vertex, storage, types, "std::vector");
        writeDeclaredMethods();
        for (size_t i = 0; i < storage.size(); ++i)
            out << "    std::vector<" << types[i] << "> " << storage[i] << ";\n";
        writeFields();
        out << "};\n";
        return {};
    }
    const auto internalColumn = [&](std::string field) {
        while (!members.insert(field).second) field += "0";
        storage.push_back(field);
        return field;
    };
    const std::string rows = model.hierarchy && !model.separateEdges ? internalColumn("links") : "";
    std::set<std::string> adjacencyColumns;
    const auto adjacency = [&](const char* base) {
        auto column = internalColumn(base);
        adjacencyColumns.insert(column);
        return column;
    };
    const auto outgoing = model.incidence ? adjacency("outgoing_") : "";
    const auto incoming = model.bidirectional ? adjacency("incoming_") : "";
    const auto childRows = model.separateEdges ? adjacency("children_") : "";
    const auto parentRows = model.separateEdges ? adjacency("parents_") : "";
    const std::string handles = alternatives.empty() ? "" : internalColumn("handles");
    // Only these columns have one entry per vertex. Each payload column grows for its own kind.
    auto allStorage = storage;
    for (const auto& alt : alternatives) allStorage.push_back(alt.member);
    if (model.keyComponentIndex) {
        allStorage.emplace_back("componentIndex_");
        headers.insert({"cocos/base/std/container/unordered_map.h", "cocos/base/std/hash/hash.h", "limits", "stdexcept"});
    }
    if (model.nameIndex) {
        allStorage.emplace_back("nameIndex_");
        headers.insert({"cocos/base/std/container/map.h", "cocos/base/std/container/string.h", "functional", "limits", "stdexcept", "string_view"});
    }

    out << '\n';
    writeDocumentation(out, node.documentation);
    out << "struct " << name << " {\n"
        << "    using " << vertex << " = " << scope.name(integer.qualifiedName) << ";\n"
        << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n";
    out << '\n';
    graphConstructors(out, source, name, allStorage, fields, copyable, model.immovable);
    GraphSections sections;
    const auto tagParam = templateParam("Tag", name, vertex, alternatives);
    if (model.vertexList) {
        headers.insert("cocos/base/GraphIterator.h");
        vertexRange(sections.vertexList.header, scope, vertex);
        sections.vertexList.header << "    " << vertex << " num_vertices() const noexcept { return static_cast<" << vertex << ">(this->" << storage.front() << ".size()); }\n";
    }
    std::string input;
    if (model.mutableGraph) {
        headers.insert("limits");
        headers.insert("stdexcept");
        if (model.nameComponentIndex) input = storage.at(*model.nameComponentIndex);
        else if (!model.components.empty() && graph.node(model.components.front().valueType).qualifiedName == "ccstd::pmr::string") input = storage.front();
        if (!input.empty()) headers.insert("string_view");
        headers.insert({"tuple", "type_traits", "utility"});
        addVertex(sections.mutation.header, sections.mutation.source, name, vertex, model.components, input, alternatives, model.hierarchy, keyColumn, keyType);
        piecewiseAddVertex(sections.mutation.header, templateDefinitions, name, vertex, model.components, storage, rows,
                           alternatives, handles, model, childRows, tagParam);
    }
    if (model.hierarchy || model.incidence || model.nameIndex || model.keyComponentIndex) {
        sections.graph.header << "    static constexpr " << vertex << " null_vertex() noexcept { return (std::numeric_limits<"
                              << vertex << ">::max)(); }\n";
    }
    if (!rows.empty()) {
        for (const auto* symbol : {"TreeEdge", "edge_descriptor", "source", "target", "out_edges", "out_degree", "edge", "out_edge_iterator", "in_edge_iterator", "adjacency_iterator",
                                   "ownership_descriptor", "children_iterator", "parent_iterator", "parent", "parent_links", "num_parents", "in_edges", "in_degree", "degree",
                                   "adjacent_vertices", "has_link"}) {
            if (members.contains(symbol))
                throw std::runtime_error(node.qualifiedName + ": shared-edge graph member name collision: " + symbol);
        }
        headers.insert({"cocos/base/GraphDescriptor.h", "cocos/base/GraphIterator.h", "cstdint", "limits", "utility"});
        hierarchyInterface(sections, scope, vertex, rows, model.edgeList);
        if (model.mutableGraph) {
            if (members.contains("add_edge") || members.contains("remove_edge"))
                throw std::runtime_error(node.qualifiedName + ": mutable graph member name collision");
            hierarchyMutation(sections.mutation.header, sections.mutation.source, name, vertex, rows, model.serializable);
        }
    }
    if (model.incidence || model.separateEdges) {
        headers.insert({"algorithm", "limits", "cocos/base/GraphDescriptor.h", "cocos/base/GraphEdges.h", "cocos/base/std/container/deque.h"});
        if (model.incidence) directedEdges(sections, scope, name, vertex, outgoing, incoming, model, false);
        if (model.separateEdges) directedEdges(sections, scope, name, vertex, childRows, parentRows, model, true);
    }
    if (model.parentGraph) {
        headers.insert("string_view");
        const auto& selected = model.components.at(model.nameComponentIndex.value());
        const auto nameAccessor = "get" + selected.publicName;
        parentLookup(sections.parent.header, sections.parent.source, name, vertex, selected.name, nameAccessor);
        if (model.addressableGraph) {
            headers.insert("stdexcept");
            headers.insert("cocos/base/std/container/string.h");
            addressableGraph(sections.addressable.header, sections.addressable.source, name, vertex, selected.name, nameAccessor);
        }
    }
    if (model.keyComponentIndex) {
        sections.uuid.header << "    bool contains(" << keyType << " key) const noexcept { return find(key) != null_vertex(); }\n"
            << "    " << vertex << " find(" << keyType << " key) const noexcept {\n"
            << "        const auto it = this->componentIndex_.find(key);\n"
            << "        return it == this->componentIndex_.end() ? null_vertex() : it->second;\n    }\n"
            << "    " << vertex << " vertex(" << keyType << " key) const { return this->componentIndex_.at(key); }\n";
        sections.mutation.header << "    void reserve(" << vertex << " capacity);\n";
        auto& definition = sections.mutation.source;
        definition << "\nvoid " << name << "::reserve(" << vertex << " capacity) {\n";
        for (const auto& column : storage)
            if (!adjacencyColumns.contains(column)) definition << "    this->" << column << ".reserve(capacity);\n";
        definition << "    this->componentIndex_.reserve(capacity);\n}\n";
    }
    if (model.removeVertex) {
        headers.insert({"algorithm", "cstddef", "type_traits"});
        removeVertex(sections.mutation.header, sections.mutation.source, name, vertex, model, storage, outgoing, incoming);
    }
    componentAccessors(sections.components.header, scope, model, vertex, storage, types, "ccstd::pmr::vector");
    if (model.nameIndex) {
        sections.uuid.header << "    bool contains(std::string_view key) const noexcept { return find(key) != null_vertex(); }\n"
            << "    " << vertex << " find(std::string_view key) const noexcept {\n"
            << "        const auto it = this->nameIndex_.find(key);\n"
            << "        return it == this->nameIndex_.end() ? null_vertex() : it->second;\n    }\n"
            << "    " << vertex << " vertex(std::string_view key) const {\n"
            << "        const auto v = find(key);\n        if (v == null_vertex()) throw std::out_of_range(\"unknown vertex name\");\n"
            << "        return v;\n    }\n";
        if (model.mutableGraph) {
            const auto i = *model.nameComponentIndex;
            sections.components.header << "    void " << nameSetter << "(" << vertex << " v, std::string_view value);\n";
            sections.components.source << "\nvoid " << name << "::" << nameSetter << "(" << vertex << " v, std::string_view value) {\n"
                   << "    auto& current = this->" << storage[i] << ".at(v);\n"
                   << "    if (std::string_view{current} == value) return;\n"
                   << "    " << types[i] << " replacement(value, current.get_allocator());\n"
                   << "    const auto old = this->nameIndex_.find(std::string_view{current});\n"
                   << "    if (!this->nameIndex_.emplace(value, v).second) throw std::invalid_argument(\"duplicate vertex name\");\n"
                   << "    current.swap(replacement);\n    this->nameIndex_.erase(old);\n}\n";
        }
    }
    if (!alternatives.empty()) {
        headers.insert({"cstdint", "type_traits", "variant", "cocos/base/TemplateUtils.h"});
        payloadAccess(sections.polymorphic.header, scope, alternatives, vertex, handles, tagParam);
    }
    sections.write(out, source);
    writeDeclaredMethods();
    out << '\n';
    if (model.compareEqual) out << "    friend bool operator==(const " << name << "&, const " << name << "&) noexcept;\n";
    if (model.compareLess) out << "    friend bool operator<(const " << name << "&, const " << name << "&) noexcept;\n";
    if (model.hashable) out << "    friend struct " << scope.name("ccstd::hash") << "<" << name << ">;\n";
    if (!alternatives.empty())
        out << "    struct VertexHandle { Kind kind; " << vertex << " index; };\n\n";
    for (size_t i = 0; i < types.size(); ++i)
        out << "    ccstd::pmr::vector<" << types[i] << "> " << storage[i] << ";\n";
    for (const auto& column : {outgoing, incoming, childRows, parentRows}) {
        if (!column.empty()) out << "    ccstd::pmr::deque<ccstd::pmr::vector<" << vertex << ">> " << column << ";\n";
    }
    if (!rows.empty()) {
        out << "    ccstd::pmr::vector<TreeEdge> " << rows << ";\n";
    }
    if (!alternatives.empty()) {
        out << "    ccstd::pmr::vector<VertexHandle> " << handles << ";\n";
        for (const auto& alt : alternatives) {
            // Keep value() addressable even when the C++ container specializes vector<bool>.
            if (alt.isBoolean) headers.insert("array");
            out << "    ccstd::pmr::vector<" << (alt.isBoolean ? "std::array<bool, 1>" : alt.type)
                << "> " << alt.member << ";\n";
        }
    }
    if (model.nameIndex)
        out << "    ccstd::pmr::map<ccstd::pmr::string, " << vertex << ", std::less<>> nameIndex_;\n";
    if (model.keyComponentIndex)
        out << "    ccstd::pmr::unordered_map<" << keyType << ", " << vertex << "> componentIndex_;\n";
    writeFields();
    if (archive) {
        const bool hasPrivateArchiveFields = std::ranges::any_of(model.fields, [](const auto& f) { return f.cppPrivate && !f.transient; });
        if (hasPrivateArchiveFields) {
            out << "    friend void save(" << scope.name(graph.node(archive->outputArchive).qualifiedName) << "&, const " << name << "&);\n";
            out << "    friend void load(" << scope.name(graph.node(archive->inputArchive).qualifiedName) << "&, " << name << "&);\n";
        }
    }
    out << "};\n";
    if (!archive) return {};
    if (transactionalLoad) {
        std::ostringstream commit;
        for (const auto& member : allStorage) commit << "    target." << member << ".swap(g." << member << ");\n";
        if (std::ranges::any_of(fields, [](const auto& f) { return !f.transient; })) commit << "    using std::swap;\n";
        for (const auto& field : fields)
            if (!field.transient) commit << "    swap(target." << field.name << ", g." << field.name << ");\n";
        return commit.str();
    }
    std::ostringstream reset;
    for (const auto& member : allStorage) reset << "    g." << member << ".clear();\n";
    for (size_t i = 0; i < fields.size(); ++i) {
        const auto& f = fields[i];
        const auto type = model.fields[i].type.type;
        if (f.transient || !archivePrimitive(graph, type).empty()) continue;
        if (graph.node(type).typescriptContainer != ContainerKind::None) reset << "    g." << f.name << ".clear();\n";
        else reset << "    g." << f.name << " = decltype(g." << f.name << "){" << (f.usesAllocator ? "g.get_allocator()" : "") << "};\n";
    }
    return reset.str();
}

} // namespace cc::codegen
