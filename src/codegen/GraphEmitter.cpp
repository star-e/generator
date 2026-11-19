#include "codegen/GraphEmitter.h"
#include "model/TypeGraphBuilder.h"
#include "model/TypeName.h"

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
                       const std::vector<std::string>& storage) {
    const auto& member = storage.front();
    out << "    explicit " << name << "(const allocator_type& alloc) noexcept";
    for (const auto& field : storage) out << (field == member ? " : " : ", ") << field << "(alloc)";
    out << " {}\n"
        << "    " << name << "(const " << name << "& rhs, const allocator_type& alloc);\n"
        << "    " << name << "(" << name << "&& rhs, const allocator_type& alloc);\n"
        << "    " << name << "(" << name << "&&) noexcept = default;\n"
        << "    " << name << "(const " << name << "&) = delete;\n";
    const auto assignment = storage.size() > 1 ? ";\n" : " = default;\n";
    out << "    " << name << "& operator=(" << name << "&&)" << assignment
        << "    " << name << "& operator=(const " << name << "&)" << assignment
        << "    allocator_type get_allocator() const noexcept { return {this->" << member << ".get_allocator().resource()}; }\n";
    source << "\n" << name << "::" << name << "(const " << name << "& rhs, const allocator_type& alloc)\n";
    for (const auto& field : storage)
        source << (field == member ? ": " : ", ") << field << "(rhs." << field << ", alloc)";
    source << " {}\n\n" << name << "::" << name << "(" << name << "&& rhs, const allocator_type& alloc)\n";
    if (storage.size() == 1) {
        source << ": " << member << "(std::move(rhs." << member << "), alloc) {}\n";
        return;
    }
    // Keep parallel component and adjacency storage together if the destination resource fails.
    source << ": " << name << "(alloc) {\n"
           << "    if (alloc.resource() == rhs.get_allocator().resource()) {\n        swapStorage(rhs);\n    } else {\n"
           << "        " << name << " copy(rhs, alloc);\n        swapStorage(copy);\n";
    for (const auto& field : storage) source << "        rhs." << field << ".clear();\n";
    source << "    }\n}\n\nvoid " << name << "::swapStorage(" << name << "& other) noexcept {\n";
    for (const auto& field : storage) source << "    this->" << field << ".swap(other." << field << ");\n";
    source << "}\n";
    for (bool move : {false, true}) {
        source << "\n" << name << "& " << name << "::operator=(" << (move ? "" : "const ") << name << (move ? "&&" : "&") << " rhs) {\n"
               << "    if (this != &rhs) {\n        " << name << " replacement(" << (move ? "std::move(rhs)" : "rhs")
               << ", get_allocator());\n        swapStorage(replacement);\n    }\n    return *this;\n}\n";
    }
}

void addVertex(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
               const std::vector<std::string>& storage, const std::string& input, const std::string& rows) {
    const bool hierarchy = !rows.empty();
    const auto& member = storage.front();
    const std::string value = input.empty() ? "" : "std::string_view value";
    out << "\n    " << vertex << " add_vertex(" << value;
    if (!value.empty()) out << " = {}";
    if (hierarchy) {
        out << ") { return add_vertex(" << (value.empty() ? "" : "value, ") << "null_vertex()); }\n"
            << "    " << vertex << " add_vertex(" << value << (value.empty() ? "" : ", ") << vertex << " parent";
    }
    out << ");\n";
    source << "\n" << name << "::" << vertex << " " << name << "::add_vertex(" << value;
    if (hierarchy) source << (value.empty() ? "" : ", ") << vertex << " parent";
    source << ") {\n";
    if (hierarchy) {
        source << "    if (parent != null_vertex() && static_cast<std::uintmax_t>(parent) >= this->" << member << ".size())\n"
               << "        throw std::out_of_range(\"invalid " << name << " parent\");\n";
    }
    source << "    if (this->" << member << ".size() >= (std::numeric_limits<" << name << "::" << vertex << ">::max)())\n"
           << "        throw std::length_error(\"" << name << " vertex limit exceeded\");\n"
           << "    const auto vertex = static_cast<" << name << "::" << vertex << ">(this->" << member << ".size());\n";
    const bool hasDefaults = storage.size() > static_cast<size_t>(hierarchy) + (input.empty() ? 0U : 1U);
    if (hasDefaults) {
        source << R"(    auto appendDefault = [](auto& items) {
        if (!items.empty() && items.size() == items.capacity()) {
            // Allocator-extended moves may throw after modifying an earlier element.
            // Grow from const copies and publish the new column only after insertion succeeds.
            std::decay_t<decltype(items)> grown(items.get_allocator());
            const auto size = items.size();
            grown.reserve(size > items.max_size() - size ? items.max_size() : size * 2);
            for (const auto& item : items) grown.emplace_back(item);
            grown.emplace_back();
            items.swap(grown);
        } else {
            items.emplace_back();
        }
    };
)";
    }
    const bool parallel = storage.size() > 1;
    const auto indent = parallel ? "        " : "    ";
    if (hierarchy) source << "    auto& edges = this->" << rows << ";\n";
    if (parallel) source << "    try {\n";
    // Consume the view before growing other columns: it can refer to text in this graph.
    if (!input.empty()) source << indent << "this->" << input << ".emplace_back(value);\n";
    for (const auto& field : storage) {
        if (field != input && field != rows) source << indent << "appendDefault(this->" << field << ");\n";
    }
    if (hierarchy) source << indent << "edges.push_back(VertexLinks{parent});\n";
    if (parallel) {
        source << "    } catch (...) {\n";
        for (const auto& field : storage) {
            source << "        if (this->" << field << ".size() > vertex) this->" << field << ".pop_back();\n";
        }
        source << "        throw;\n    }\n";
    }
    if (hierarchy) {
        source << R"(    if (parent != null_vertex()) {
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
    source << "    return vertex;\n}\n";
}

} // namespace

void emitCppGraph(const TypeGraph& graph, TypeId id, std::ostream& out, std::ostream& source,
                  std::set<std::string>& headers, std::set<std::string>& forwards) {
    const auto& node = graph.node(id);
    const auto& model = std::get<GraphType>(node.data);
    if (!model.indexedVertices || model.components.empty())
        throw std::runtime_error(node.qualifiedName + ": C++ graph output requires indexed storage and at least one component");
    // The descriptor alias is a scoped declaration. Its resolved integer type is stored on the graph.
    const auto children = graph.out_edges(id.value);
    const auto alias = std::ranges::find_if(children, [&](auto edge) {
        return std::holds_alternative<AliasType>(graph.node(TypeId{graph.target(edge)}).data);
    });
    if (alias == children.end()) throw std::runtime_error(node.qualifiedName + ": missing vertex descriptor alias");
    const auto& vertex = graph.node(TypeId{graph.target(*alias)}).name;
    const auto& integer = graph.node(model.vertexType);
    const auto& name = node.name;
    headers.insert("cocos/base/std/container/vector.h");
    if (!integer.cppHeader.empty()) headers.insert(integer.cppHeader);
    std::vector<std::string> storage;
    std::vector<std::string> types;
    for (const auto& component : model.components) {
        storage.push_back(component.name);
        types.push_back(formatTypeName(graph, component.valueType, [&](const TypeNode& type) {
            if (!type.cppHeader.empty()) headers.insert(type.cppHeader);
            return type.qualifiedName;
        }));
        const auto& tag = graph.node(component.tag);
        if (component.tag.value > id.value || tag.module != node.module) {
            const auto ns = tag.qualifiedName.rfind("::");
            const auto declaration = "struct " + tag.name + ";";
            forwards.insert(ns == std::string::npos ? declaration : "namespace " + tag.qualifiedName.substr(0, ns) + " { " + declaration + " }");
        }
    }
    std::string rows;
    if (model.hierarchy) {
        rows = "links_";
        for (size_t i = 0; std::ranges::find(storage, rows) != storage.end(); ++i)
            rows = "adjacency" + std::to_string(i) + "_";
        storage.push_back(rows);
    }

    out << "\nstruct " << name << " {\n"
        << "    using " << vertex << " = " << integer.qualifiedName << ";\n"
        << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n\n";
    graphConstructors(out, source, name, storage);
    if (model.vertexList) {
        headers.insert("cstddef");
        headers.insert("iterator");
        vertexRange(out, vertex);
        out << "    " << vertex << " num_vertices() const noexcept { return static_cast<" << vertex << ">(this->" << storage.front() << ".size()); }\n";
    }
    if (model.mutableGraph) {
        headers.insert("limits");
        headers.insert("stdexcept");
        std::string input;
        if (model.nameComponentIndex) input = storage.at(*model.nameComponentIndex);
        else if (types.front() == "ccstd::pmr::string") input = storage.front();
        if (!input.empty()) headers.insert("string_view");
        if (input.empty() || types.size() > 1) headers.insert("type_traits");
        addVertex(out, source, name, vertex, storage, input, rows);
    }
    if (model.hierarchy) {
        headers.insert({"cstddef", "cstdint", "iterator", "limits"});
        hierarchyInterface(out, name, vertex, rows);
    }
    if (model.parentGraph) {
        headers.insert("string_view");
        const auto& selected = model.components.at(model.nameComponentIndex.value());
        const auto nameAccessor = "get" + componentAccessorSuffix(graph.node(selected.tag).name);
        parentLookup(out, source, name, vertex, selected.name, nameAccessor);
        if (model.addressableGraph) {
            headers.insert("stdexcept");
            headers.insert("cocos/base/std/container/string.h");
            addressableGraph(out, source, name, vertex, selected.name, nameAccessor);
        }
    }
    for (size_t i = 0; i < model.components.size(); ++i) {
        const auto& tag = graph.node(model.components[i].tag);
        const auto accessor = "get" + componentAccessorSuffix(tag.name);
        for (const std::string qualifier : {"", " const"}) {
            const auto result = types[i] == "bool"
                ? "ccstd::pmr::vector<bool>::" + std::string(qualifier.empty() ? "reference" : "const_reference")
                : types[i] + qualifier + "&";
            out << "\n    " << result << " " << accessor << "(" << vertex << " v)" << qualifier
                << " noexcept { return this->" << storage[i] << "[v]; }\n"
                << "    " << result << " get(const ::" << tag.qualifiedName << "&, " << vertex << " v)"
                << qualifier << " noexcept { return " << accessor << "(v); }\n";
        }
    }
    out << "\nprivate:\n";
    for (size_t i = 0; i < types.size(); ++i)
        out << "    ccstd::pmr::vector<" << types[i] << "> " << storage[i] << ";\n";
    if (model.hierarchy) {
        out << "    struct VertexLinks {\n"
            << "        " << vertex << " parent{null_vertex()};\n"
            << "        " << vertex << " firstChild{null_vertex()};\n"
            << "        " << vertex << " lastChild{null_vertex()};\n"
            << "        " << vertex << " nextSibling{null_vertex()};\n"
            << "        " << vertex << " childCount{};\n    };\n"
            << "    ccstd::pmr::vector<VertexLinks> " << rows << ";\n";
    }
    if (storage.size() > 1) out << "    void swapStorage(" << name << "& other) noexcept;\n";
    out << "};\n";
}

} // namespace cc::codegen
