#include "codegen/GraphEmitter.h"
#include "model/TypeGraphBuilder.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <string_view>

namespace cc::codegen {
namespace {

void vertexRange(std::ostream& out, const std::string& vertex) {
    out << "\n    class VertexIterator {\n    public:\n"
        << "        using value_type = " << vertex << ";\n"
        << R"(        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using pointer = void;
        using reference = value_type;

        VertexIterator() = default;
        explicit VertexIterator(value_type value) noexcept : current(value) {}
        value_type operator*() const noexcept { return current; }
        VertexIterator& operator++() noexcept { ++current; return *this; }
        VertexIterator operator++(int) noexcept { auto previous = *this; ++*this; return previous; }
        bool operator==(VertexIterator rhs) const noexcept { return current == rhs.current; }
        bool operator!=(VertexIterator rhs) const noexcept { return !(*this == rhs); }

    private:
        value_type current{};
    };

    struct VertexRange {
        VertexIterator begin() const noexcept { return VertexIterator{}; }
        VertexIterator end() const noexcept { return last; }
        VertexIterator last;
    };

    VertexRange vertices() const noexcept { return {VertexIterator(num_vertices())}; }
)";
}

void hierarchyInterface(std::ostream& out, const std::string& name, const std::string& vertex,
                        const std::string& rows) {
    out << "\n    static constexpr " << vertex << " null_vertex() noexcept { return (std::numeric_limits<"
        << vertex << ">::max)(); }\n"
        << "    struct Link { " << vertex << " source, target; };\n"
        << "    " << vertex << " child(Link link) const noexcept { return link.target; }\n"
        << "    " << vertex << " get_parent(" << vertex << " v) const noexcept { return v == null_vertex() ? v : this->"
        << rows << "[v].parent; }\n"
        << "    " << vertex << " num_children(" << vertex << " v) const noexcept { return this->" << rows << "[v].childCount; }\n"
        << "\n    class ChildLinkIterator {\n    public:\n"
        << R"(        using value_type = Link;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using pointer = void;
        using reference = Link;

        ChildLinkIterator() = default;
)"
        << "        ChildLinkIterator(const " << name << "* graph, " << vertex << " v) noexcept : owner(graph), current(v) {}\n"
        << "        Link operator*() const noexcept { return {owner->get_parent(current), current}; }\n"
        << "        ChildLinkIterator& operator++() noexcept { current = owner->" << rows << "[current].nextSibling; return *this; }\n"
        << R"(        ChildLinkIterator operator++(int) noexcept { auto previous = *this; ++*this; return previous; }
        bool operator==(ChildLinkIterator rhs) const noexcept { return owner == rhs.owner && current == rhs.current; }
        bool operator!=(ChildLinkIterator rhs) const noexcept { return !(*this == rhs); }
    private:
)"
        << "        const " << name << "* owner{};\n        " << vertex << " current{null_vertex()};\n    };\n"
        << R"(
    struct ChildLinkRange {
        ChildLinkIterator first, last;
        ChildLinkIterator begin() const noexcept { return first; }
        ChildLinkIterator end() const noexcept { return last; }
    };
)"
        << "    ChildLinkRange child_links(" << vertex << " v) const noexcept {\n"
        << "        return {{this, this->" << rows << "[v].firstChild}, {this, null_vertex()}};\n    }\n";
}

void parentLookup(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
                  const ComponentInfo& component, const TypeGraph& graph) {
    const auto accessor = "get" + componentAccessorSuffix(graph.node(component.tag).name);
    out << "    " << vertex << " locate_child(" << vertex << " parent, std::string_view name) const noexcept;\n";
    source << "\n" << name << "::" << vertex << " " << name << "::locate_child(" << vertex
           << " parent, std::string_view name) const noexcept {\n"
           << "    const auto count = this->" << component.name << ".size();\n"
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

void graphConstructors(std::ostream& out, std::ostream& source, const std::string& name,
                       const std::string& member, const std::string& rows) {
    const bool hierarchy = !rows.empty();
    out << "    explicit " << name << "(const allocator_type& alloc) noexcept : " << member << "(alloc)";
    if (hierarchy) out << ", " << rows << "(alloc)";
    out << " {}\n"
        << "    " << name << "(const " << name << "& rhs, const allocator_type& alloc);\n"
        << "    " << name << "(" << name << "&& rhs, const allocator_type& alloc);\n"
        << "    " << name << "(" << name << "&&) noexcept = default;\n"
        << "    " << name << "(const " << name << "&) = delete;\n";
    const auto assignment = hierarchy ? ";\n" : " = default;\n";
    out << "    " << name << "& operator=(" << name << "&&)" << assignment
        << "    " << name << "& operator=(const " << name << "&)" << assignment
        << "    allocator_type get_allocator() const noexcept { return {this->" << member << ".get_allocator().resource()}; }\n";
    source << "\n" << name << "::" << name << "(const " << name << "& rhs, const allocator_type& alloc)\n"
           << ": " << member << "(rhs." << member << ", alloc)";
    if (hierarchy) source << ", " << rows << "(rhs." << rows << ", alloc)";
    source << " {}\n\n" << name << "::" << name << "(" << name << "&& rhs, const allocator_type& alloc)\n";
    if (!hierarchy) {
        source << ": " << member << "(std::move(rhs." << member << "), alloc) {}\n";
        return;
    }
    // Keep parallel component and adjacency storage together if the destination resource fails.
    source << ": " << name << "(alloc) {\n"
           << "    if (alloc.resource() == rhs.get_allocator().resource()) {\n        swapStorage(rhs);\n    } else {\n"
           << "        " << name << " copy(rhs, alloc);\n        swapStorage(copy);\n"
           << "        rhs." << rows << ".clear();\n        rhs." << member << ".clear();\n    }\n}\n"
           << "\nvoid " << name << "::swapStorage(" << name << "& other) noexcept {\n"
           << "    this->" << member << ".swap(other." << member << ");\n"
           << "    this->" << rows << ".swap(other." << rows << ");\n}\n";
    for (bool move : {false, true}) {
        source << "\n" << name << "& " << name << "::operator=(" << (move ? "" : "const ") << name << (move ? "&&" : "&") << " rhs) {\n"
               << "    if (this != &rhs) {\n        " << name << " replacement(" << (move ? "std::move(rhs)" : "rhs")
               << ", get_allocator());\n        swapStorage(replacement);\n    }\n    return *this;\n}\n";
    }
}

void addVertex(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
               const std::string& member, const std::string& rows) {
    const bool hierarchy = !rows.empty();
    out << "\n    " << vertex << " add_vertex(std::string_view value = {})";
    if (hierarchy) {
        out << " { return add_vertex(value, null_vertex()); }\n"
            << "    " << vertex << " add_vertex(std::string_view value, " << vertex << " parent)";
    }
    out << ";\n";
    source << "\n" << name << "::" << vertex << " " << name << "::add_vertex(std::string_view value";
    if (hierarchy) source << ", " << vertex << " parent";
    source << ") {\n";
    if (hierarchy) {
        source << "    if (parent != null_vertex() && static_cast<std::uintmax_t>(parent) >= this->" << member << ".size())\n"
               << "        throw std::out_of_range(\"invalid " << name << " parent\");\n";
    }
    source << "    if (this->" << member << ".size() >= (std::numeric_limits<" << name << "::" << vertex << ">::max)())\n"
           << "        throw std::length_error(\"" << name << " vertex limit exceeded\");\n"
           << "    const auto vertex = static_cast<" << name << "::" << vertex << ">(this->" << member << ".size());\n";
    if (hierarchy) {
        source << "    auto& edges = this->" << rows << ";\n"
               << "    edges.push_back(VertexLinks{parent});\n"
               << "    try {\n        this->" << member << ".emplace_back(value);\n"
               << R"(    } catch (...) {
        edges.pop_back();
        throw;
    }
    if (parent != null_vertex()) {
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
    } else {
        source << "    this->" << member << ".emplace_back(value);\n";
    }
    source << "    return vertex;\n}\n";
}

} // namespace

void emitCppGraph(const TypeGraph& graph, TypeId id, std::ostream& out, std::ostream& source,
                  std::set<std::string>& headers, std::set<std::string>& forwards) {
    const auto& node = graph.node(id);
    const auto& model = std::get<GraphType>(node.data);
    if (!model.indexedVertices || model.components.size() != 1)
        throw std::runtime_error(node.qualifiedName + ": C++ graph output currently requires indexed storage and one component");
    const auto& component = model.components.front();
    const auto& value = graph.node(component.valueType);
    if (value.qualifiedName != "ccstd::pmr::string" || value.cppHeader.empty())
        throw std::runtime_error(node.qualifiedName + ": expected a ccstd::pmr::string component with a native header");
    // The descriptor alias is a scoped declaration. Its resolved integer type is stored on the graph.
    const auto children = graph.out_edges(id.value);
    const auto alias = std::ranges::find_if(children, [&](auto edge) {
        return std::holds_alternative<AliasType>(graph.node(TypeId{graph.target(edge)}).data);
    });
    if (alias == children.end()) throw std::runtime_error(node.qualifiedName + ": missing vertex descriptor alias");
    const auto& vertex = graph.node(TypeId{graph.target(*alias)}).name;
    const auto& integer = graph.node(model.vertexType);
    const auto& tag = graph.node(component.tag);
    const auto& member = component.name;
    const auto accessor = "get" + componentAccessorSuffix(tag.name);
    const auto& name = node.name;
    std::string rows;
    if (model.hierarchy) rows = member == "links_" ? "adjacency_" : "links_";
    headers.insert("cocos/base/std/container/vector.h");
    headers.insert(value.cppHeader);
    if (!integer.cppHeader.empty()) headers.insert(integer.cppHeader);
    if (component.tag.value > id.value || tag.module != node.module) {
        const auto ns = tag.qualifiedName.rfind("::");
        const auto declaration = "struct " + tag.name + ";";
        forwards.insert(ns == std::string::npos ? declaration : "namespace " + tag.qualifiedName.substr(0, ns) + " { " + declaration + " }");
    }

    out << "\nstruct " << name << " {\n"
        << "    using " << vertex << " = " << integer.qualifiedName << ";\n"
        << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n\n";
    graphConstructors(out, source, name, member, rows);
    if (model.vertexList) {
        headers.insert("cstddef");
        headers.insert("iterator");
        vertexRange(out, vertex);
        out << "    " << vertex << " num_vertices() const noexcept { return static_cast<" << vertex << ">(this->" << member << ".size()); }\n";
    }
    if (model.mutableGraph) {
        headers.insert("string_view");
        headers.insert("limits");
        headers.insert("stdexcept");
        addVertex(out, source, name, vertex, member, rows);
    }
    if (model.hierarchy) {
        headers.insert({"cstddef", "cstdint", "iterator", "limits"});
        hierarchyInterface(out, name, vertex, rows);
    }
    if (model.parentGraph) {
        headers.insert("string_view");
        parentLookup(out, source, name, vertex, model.components.at(model.nameComponentIndex.value()), graph);
    }
    for (const auto qualifier : {std::string_view{}, std::string_view{" const"}}) {
        out << "\n    " << value.qualifiedName << qualifier << "& " << accessor << "(" << vertex << " v)" << qualifier
            << " noexcept { return this->" << member << "[v]; }\n"
            << "    " << value.qualifiedName << qualifier << "& get(const ::" << tag.qualifiedName << "&, " << vertex << " v)"
            << qualifier << " noexcept { return " << accessor << "(v); }\n";
    }
    out << "\nprivate:\n    ccstd::pmr::vector<" << value.qualifiedName << "> " << member << ";\n";
    if (model.hierarchy) {
        out << "    struct VertexLinks {\n"
            << "        " << vertex << " parent{null_vertex()};\n"
            << "        " << vertex << " firstChild{null_vertex()};\n"
            << "        " << vertex << " lastChild{null_vertex()};\n"
            << "        " << vertex << " nextSibling{null_vertex()};\n"
            << "        " << vertex << " childCount{};\n    };\n"
            << "    ccstd::pmr::vector<VertexLinks> " << rows << ";\n"
            << "    void swapStorage(" << name << "& other) noexcept;\n";
    }
    out << "};\n";
}

} // namespace cc::codegen
