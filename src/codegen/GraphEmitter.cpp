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

void componentAccessors(std::ostream& out, const TypeGraph& graph, const GraphType& model, const std::string& vertex,
                        const std::vector<std::string>& storage, const std::vector<std::string>& types,
                        std::string_view vectorType) {
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
                << "    " << result << " get(const ::" << tag.qualifiedName << "&, " << vertex << " v)"
                << qualifier << " noexcept { return " << accessor << "(v); }\n";
        }
    }
}

void hierarchyInterface(std::ostream& out, const std::string& name, const std::string& vertex,
                        const std::string& rows) {
    out << "    struct Link { " << vertex << " source, target; };\n"
        << "    " << vertex << " child(Link link) const noexcept { return link.target; }\n"
        << "    " << vertex << " get_parent(" << vertex << " v) const noexcept { return v == null_vertex() ? v : this->"
        << rows << "[v].parent; }\n"
        << "    " << vertex << " num_children(" << vertex << " v) const noexcept { return this->" << rows << "[v].childCount; }\n"
        << "\n    template<bool Adjacent>\n    class ChildIterator {\n    public:\n"
        << "        using value_type = std::conditional_t<Adjacent, " << vertex << ", Link>;\n"
        << R"(        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using pointer = void;
        using reference = value_type;

        ChildIterator() = default;
)"
        << "        ChildIterator(const " << name << "* graph, " << vertex << " v) noexcept : owner(graph), current(v) {}\n"
        << "        value_type operator*() const noexcept {\n            if constexpr (Adjacent) return current;\n"
        << "            else return {owner->get_parent(current), current};\n        }\n"
        << "        ChildIterator& operator++() noexcept { current = owner->" << rows << "[current].nextSibling; return *this; }\n"
        << R"(        ChildIterator operator++(int) noexcept { auto previous = *this; ++*this; return previous; }
        bool operator==(ChildIterator rhs) const noexcept { return owner == rhs.owner && current == rhs.current; }
        bool operator!=(ChildIterator rhs) const noexcept { return !(*this == rhs); }
    private:
)"
        << "        const " << name << "* owner{};\n        " << vertex << " current{null_vertex()};\n    };\n"
        << R"(
    using ChildLinkIterator = ChildIterator<false>;
    template<class Iterator>
    struct IteratorRange {
        Iterator first, last;
        Iterator begin() const noexcept { return first; }
        Iterator end() const noexcept { return last; }
    };
    using ChildLinkRange = IteratorRange<ChildLinkIterator>;
)"
        << "    ChildLinkRange child_links(" << vertex << " v) const noexcept {\n"
        << "        return {{this, this->" << rows << "[v].firstChild}, {this, null_vertex()}};\n    }\n"
        << "\n    using Edge = Link;\n"
        << "    " << vertex << " source(Edge e) const noexcept { return e.source; }\n"
        << "    " << vertex << " target(Edge e) const noexcept { return e.target; }\n"
        << "    ChildLinkRange out_edges(" << vertex << " v) const noexcept { return child_links(v); }\n"
        << "    " << vertex << " out_degree(" << vertex << " v) const noexcept { return num_children(v); }\n"
        << "    " << vertex << " parent(Link e) const noexcept { return e.source; }\n"
        << "    " << vertex << " num_parents(" << vertex << " v) const noexcept { return get_parent(v) != null_vertex(); }\n"
        << R"(    struct ParentLinkIterator {
        using value_type = Link;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using pointer = void;
        using reference = Link;
        Link link{null_vertex(), null_vertex()};
        Link operator*() const noexcept { return link; }
        ParentLinkIterator& operator++() noexcept { link.source = null_vertex(); return *this; }
        ParentLinkIterator operator++(int) noexcept { auto previous = *this; ++*this; return previous; }
        bool operator==(ParentLinkIterator rhs) const noexcept { return link.source == rhs.link.source && link.target == rhs.link.target; }
        bool operator!=(ParentLinkIterator rhs) const noexcept { return !(*this == rhs); }
    };
    using ParentLinkRange = IteratorRange<ParentLinkIterator>;
)"
        << "    ParentLinkRange parent_links(" << vertex << " v) const noexcept { return {{{get_parent(v), v}}, {{null_vertex(), v}}}; }\n"
        << "    ParentLinkRange in_edges(" << vertex << " v) const noexcept { return parent_links(v); }\n"
        << "    " << vertex << " in_degree(" << vertex << " v) const noexcept { return num_parents(v); }\n"
        << "    " << vertex << " degree(" << vertex << " v) const noexcept { return in_degree(v) + out_degree(v); }\n"
        << "    IteratorRange<ChildIterator<true>> adjacent_vertices(" << vertex << " v) const noexcept {\n"
        << "        return {{this, this->" << rows << "[v].firstChild}, {this, null_vertex()}};\n    }\n"
        << "    " << vertex << " num_edges() const noexcept {\n        " << vertex << " count{};\n"
        << "        for (const auto& row : this->" << rows << ") count += row.childCount;\n        return count;\n    }\n"
        << "    std::pair<Edge, bool> edge(" << vertex << " u, " << vertex << " v) const noexcept {\n"
        << "        if (u != null_vertex() && static_cast<std::uintmax_t>(v) < this->" << rows
        << ".size() && get_parent(v) == u) return {{u, v}, true};\n"
        << "        return {{null_vertex(), null_vertex()}, false};\n    }\n";
    out << "    bool has_link(" << vertex << " u, " << vertex << " v) const noexcept { return edge(u, v).second; }\n";
    out << "\n    class EdgeIterator {\n    public:\n"
        << R"(        using value_type = Edge;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;
        using pointer = void;
        using reference = Edge;
        EdgeIterator() = default;
)"
        << "        EdgeIterator(const " << name << "* graph, " << vertex << " first) noexcept : owner(graph), u(first) { seek(); }\n"
        << "        Edge operator*() const noexcept { return {u, v}; }\n"
        << "        EdgeIterator& operator++() noexcept {\n            v = owner->" << rows << "[v].nextSibling;\n"
        << "            if (v == null_vertex()) { ++u; seek(); }\n            return *this;\n        }\n"
        << R"(        EdgeIterator operator++(int) noexcept { auto previous = *this; ++*this; return previous; }
        bool operator==(EdgeIterator rhs) const noexcept { return owner == rhs.owner && u == rhs.u && v == rhs.v; }
        bool operator!=(EdgeIterator rhs) const noexcept { return !(*this == rhs); }
    private:
        void seek() noexcept {
)"
        << "            while (u < owner->" << rows << ".size()) {\n                v = owner->" << rows << "[u].firstChild;\n"
        << "                if (v != null_vertex()) break;\n                ++u;\n            }\n        }\n"
        << "        const " << name << "* owner{};\n        " << vertex << " u{}, v{null_vertex()};\n    };\n"
        << "    IteratorRange<EdgeIterator> edges() const noexcept {\n"
        << "        return {{this, 0}, {this, static_cast<" << vertex << ">(this->" << rows << ".size())}};\n    }\n";
}

void directedEdges(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
                   const std::string& outgoing, const std::string& incoming, bool mutableGraph, bool references) {
    const std::string descriptor = references ? "Link" : "Edge";
    const std::string operation = references ? "reference" : "edge";
    out << "    using " << descriptor << " = ::cc::detail::DirectedEdge<" << vertex << ">;\n"
        << "    " << vertex << (references ? " parent(" : " source(") << descriptor << " e) const noexcept { return e.source; }\n"
        << "    " << vertex << (references ? " child(" : " target(") << descriptor << " e) const noexcept { return e.target; }\n";
    const auto incident = [&](const char* range, const char* count, const std::string& rows, bool reversed) {
        out << "    auto " << range << "(" << vertex << " v) const noexcept {\n"
            << "        return ::cc::detail::DirectedEdgeRange<" << (reversed ? "true" : "false") << ", decltype(" << rows
            << ")>{" << rows << ", v, static_cast<std::size_t>(v) + 1};\n    }\n"
            << "    std::size_t " << count << "(" << vertex << " v) const noexcept { return " << rows << "[v].size(); }\n";
    };
    incident(references ? "child_links" : "out_edges", references ? "num_children" : "out_degree", outgoing, false);
    if (!incoming.empty()) {
        incident(references ? "parent_links" : "in_edges", references ? "num_parents" : "in_degree", incoming, true);
        if (references)
            out << "    " << vertex << " get_parent(" << vertex << " v) const noexcept {\n"
                << "        return v >= " << incoming << ".size() || " << incoming << "[v].empty() ? null_vertex() : " << incoming << "[v].front();\n    }\n";
        else out << "    std::size_t degree(" << vertex << " v) const noexcept { return in_degree(v) + out_degree(v); }\n";
    }
    if (!references) out << "    const auto& adjacent_vertices(" << vertex << " v) const noexcept { return " << outgoing << "[v]; }\n"
        << "    auto edges() const noexcept { return ::cc::detail::DirectedEdgeRange<false, decltype(" << outgoing
        << ")>{" << outgoing << ", 0, " << outgoing << ".size()}; }\n"
        << "    std::size_t num_edges() const noexcept {\n        std::size_t count{};\n"
        << "        for (const auto& row : " << outgoing << ") count += row.size();\n        return count;\n    }\n";
    out << "    std::pair<" << descriptor << ", bool> " << operation << "(" << vertex << " u, " << vertex << " v) const noexcept {\n"
        << "        if (u < " << outgoing << ".size()) {\n            const auto& row = " << outgoing << "[u];\n"
        << "            if (std::find(row.begin(), row.end(), v) != row.end()) return {{u, v}, true};\n        }\n"
        << "        return {{null_vertex(), null_vertex()}, false};\n    }\n";
    if (references) out << "    bool has_link(" << vertex << " u, " << vertex << " v) const noexcept { return reference(u, v).second; }\n";
    if (!mutableGraph) return;
    out << "    std::pair<" << descriptor << ", bool> add_" << operation << "(" << vertex << " u, " << vertex << " v);\n"
        << "    void remove_" << operation << "(" << descriptor << " e) noexcept;\n"
        << "    void remove_" << operation << "(" << vertex << " u, " << vertex << " v) noexcept;\n";
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
    out << "    std::pair<Edge, bool> add_edge(" << vertex << " u, " << vertex << " v) noexcept;\n"
        << "    void remove_edge(Edge e) noexcept;\n";
    source << "\nstd::pair<" << name << "::Edge, bool> " << name << "::add_edge(" << vertex << " u, " << vertex << " v) noexcept {\n"
           << "    auto& links = this->" << rows << ";\n"
           << "    if (static_cast<std::uintmax_t>(u) >= links.size() || static_cast<std::uintmax_t>(v) >= links.size() ||\n"
           << "        links[v].parent != null_vertex()" << (serializable ? " || u >= v" : "") << ") return {{null_vertex(), null_vertex()}, false};\n";
    if (!serializable)
        source << "    for (auto p = u; p != null_vertex(); p = links[p].parent)\n"
               << "        if (p == v) return {{null_vertex(), null_vertex()}, false};\n";
    source << R"(    auto& parent = links[u];
    auto* slot = &parent.firstChild;
    while (*slot != null_vertex() && *slot < v) slot = &links[*slot].nextSibling;
    links[v].nextSibling = *slot;
    if (*slot == null_vertex()) parent.lastChild = v;
    *slot = v;
    links[v].parent = u;
    ++parent.childCount;
    return {{u, v}, true};
}
)";
    source << "\nvoid " << name << "::remove_edge(Edge e) noexcept {\n"
           << "    if (!edge(e.source, e.target).second) return;\n"
           << "    auto& links = this->" << rows << ";\n"
           << R"(    auto& parent = links[e.source];
    auto& child = links[e.target];
    auto previous = null_vertex();
    auto* slot = &parent.firstChild;
    while (*slot != e.target) {
        previous = *slot;
        slot = &links[previous].nextSibling;
    }
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
                       const std::vector<std::string>& storage, const std::set<std::string>& moveOnlyColumns,
                       const std::vector<BoundField>& fields, const GraphType& model, bool transactionalArchive) {
    const auto writeSwap = [&] {
        const bool skipTransients = transactionalArchive && std::ranges::any_of(fields, &BoundField::transient);
        source << "\nvoid " << name << "::swapStorage(" << name << "& other"
               << (skipTransients ? ", bool includeTransient" : "") << ")" << (fields.empty() ? " noexcept" : "") << " {\n";
        for (const auto& field : storage) source << "    this->" << field << ".swap(other." << field << ");\n";
        if (!fields.empty()) source << "    using std::swap;\n";
        for (const auto& f : fields)
            source << "    " << (skipTransients && f.transient ? "if (includeTransient) " : "")
                   << "swap(this->" << f.name << ", other." << f.name << ");\n";
        source << "}\n";
    };
    const auto& member = storage.front();
    const bool copyable = moveOnlyColumns.empty();
    out << "    explicit " << name << "(const allocator_type& alloc)";
    for (const auto& field : storage)
        out << (field == member ? " : " : ", ") << field << "(alloc)";
    for (const auto& f : fields)
        if (f.usesAllocator) out << ", " << f.name << "(alloc)";
    out << " {}\n";
    if (model.immovable) {
        out << "    " << name << "(const " << name << "&) = delete;\n"
            << "    " << name << "(" << name << "&&) = delete;\n"
            << "    " << name << "& operator=(const " << name << "&) = delete;\n"
            << "    " << name << "& operator=(" << name << "&&) = delete;\n"
            << "    allocator_type get_allocator() const noexcept { return {this->" << member << ".get_allocator().resource()}; }\n";
        if (transactionalArchive) writeSwap();
        return;
    }
    out << "    " << name << "(const " << name << "& rhs, const allocator_type& alloc)" << (copyable ? ";\n" : " = delete;\n")
        << "    " << name << "(" << name << "&& rhs, const allocator_type& alloc);\n"
        << "    " << name << "(" << name << "&& rhs) noexcept : "
        << name << "(std::move(rhs), rhs.get_allocator()) {}\n"
        << "    " << name << "(const " << name << "&) = delete;\n";
    const bool simple = storage.size() == 1 && copyable && fields.empty();
    const auto assignment = simple ? " = default;\n" : ";\n";
    out << "    " << name << "& operator=(" << name << "&&)" << assignment
        << "    " << name << "& operator=(const " << name << "&)" << (copyable ? assignment : " = delete;\n")
        << "    allocator_type get_allocator() const noexcept { return {this->" << member << ".get_allocator().resource()}; }\n";
    if (copyable) {
        source << "\n" << name << "::" << name << "(const " << name << "& rhs, const allocator_type& alloc)\n";
        for (const auto& field : storage)
            source << (field == member ? ": " : ", ") << field << "(rhs." << field << ", alloc)";
        for (const auto& f : fields) source << ", " << f.name << "(rhs." << f.name << (f.usesAllocator ? ", alloc)" : ")");
        source << " {}\n";
    }
    source << "\n" << name << "::" << name << "(" << name << "&& rhs, const allocator_type& alloc)\n";
    if (simple) {
        source << ": " << member << "(std::move(rhs." << member << "), alloc) {}\n";
        return;
    }
    // Keep parallel component and adjacency storage together if the destination resource fails.
    source << ": " << name << "(alloc) {\n"
           << "    if (alloc == rhs.get_allocator()) {\n        swapStorage(rhs);\n    } else {\n";
    if (copyable) {
        source << "        " << name << " copy(rhs, alloc);\n        swapStorage(copy);\n";
    } else {
        // Build the copyable columns first. Clear the source topology only after every transfer succeeds.
        for (const auto& field : storage) {
            if (!moveOnlyColumns.contains(field))
                source << "        decltype(" << field << ")(rhs." << field << ", alloc).swap(this->" << field << ");\n";
        }
        for (const auto& field : storage)
            if (moveOnlyColumns.contains(field))
                source << "        for (auto& item : rhs." << field << ") this->" << field << ".emplace_back(std::move(item));\n";
        for (const auto& f : fields) {
            source << "        this->" << f.name << " = ";
            if (f.usesAllocator) source << f.cpp << "(std::move(rhs." << f.name << "), alloc)";
            else source << "std::move(rhs." << f.name << ")";
            source << ";\n";
        }
    }
    for (const auto& field : storage) source << "        rhs." << field << ".clear();\n";
    source << "    }\n}\n";
    writeSwap();
    for (bool move : {false, true}) {
        if (!move && !copyable) continue;
        source << "\n" << name << "& " << name << "::operator=(" << (move ? "" : "const ") << name << (move ? "&&" : "&") << " rhs) {\n"
               << "    if (this != &rhs) {\n        " << name << " replacement(" << (move ? "std::move(rhs)" : "rhs")
               << ", get_allocator());\n        swapStorage(replacement);\n    }\n    return *this;\n}\n";
    }
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

void payloadAccess(std::ostream& out, const std::vector<Alternative>& alternatives,
                   const std::string& vertex, const std::string& handles) {
    const auto tagParam = vertex == "Tag" ? "VertexTag" : "Tag";
    out << "\n    enum class Kind { ";
    for (size_t i = 0; i < alternatives.size(); ++i) out << (i ? ", " : "") << alternatives[i].kind;
    out << " };\n    Kind kind(" << vertex << " v) const { return this->" << handles << ".at(v).kind; }\n"
        << "    template<class " << tagParam << ">\n    bool holds(" << vertex << " v) const noexcept {\n"
        << "        return static_cast<std::uintmax_t>(v) < this->" << handles << ".size() && this->" << handles
        << "[v].kind == kindOf(static_cast<" << tagParam << "*>(nullptr));\n    }\n";
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
            << "            auto handlers = ::cc::overloaded{std::forward<" << visitorType << ">(visitor), std::forward<" << othersType << ">(others)...};\n"
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
    std::string value, arguments;
    if (!keyColumn.empty()) {
        value = keyType + " key";
        arguments = "key";
    }
    if (!input.empty()) {
        value += value.empty() ? "std::string_view value" : ", std::string_view value";
        arguments += arguments.empty() ? "value" : ", value";
    }
    const std::string parameters = value + (hierarchy ? (value.empty() ? "" : ", ") + vertex + " parent" : "");
    const std::string definition = "\n" + name + "::" + vertex + " " + name + "::add_vertex(";
    out << "\n    " << vertex << " add_vertex(" << value;
    if (!input.empty()) out << " = {}";
    if (hierarchy) {
        out << ") { return add_vertex(" << (arguments.empty() ? "" : arguments + ", ") << "null_vertex()); }\n"
            << "    " << vertex << " add_vertex(" << parameters;
    }
    out << ");\n";
    source << definition << parameters << ") {\n";
    if (!alternatives.empty()) {
        source << "    return add_vertex(" << alternatives.front().tag << "{}" << (arguments.empty() ? "" : ", " + arguments)
               << (hierarchy ? ", parent" : "") << ");\n}\n";
    }
    const auto forwardDefaults = [&](bool tagged) {
        std::vector<std::string> args;
        if (tagged) args.emplace_back("tag");
        for (const auto& component : components)
            args.emplace_back(component.name == input ? "std::forward_as_tuple(value)" :
                              component.name == keyColumn ? "std::forward_as_tuple(key)" : "std::tuple<>{}");
        if (tagged) args.emplace_back("std::tuple<>{}");
        if (hierarchy) args.emplace_back("parent");
        source << "    return emplace_vertex(";
        for (size_t i = 0; i < args.size(); ++i) source << (i ? ", " : "") << args[i];
        source << ");\n}\n";
    };
    if (alternatives.empty()) forwardDefaults(false);
    for (const auto& alt : alternatives) {
        out << "    " << vertex << " add_vertex(const " << alt.tag << "&";
        if (!value.empty()) out << ", " << value;
        if (hierarchy) out << ", " << vertex << " parent = null_vertex()";
        out << ");\n";
        source << definition << "const " << alt.tag << "& tag" << (parameters.empty() ? "" : ", ") << parameters << ") {\n";
        forwardDefaults(true);
    }
}

void emplaceVertex(std::ostream& out, std::ostream& impl, const std::string& name, const std::string& vertex,
                   const std::vector<ComponentInfo>& components, const std::vector<std::string>& storage,
                   const std::vector<std::string>& types, const std::set<std::string>& dequeColumns,
                   const std::string& input, const std::string& rows, const std::vector<Alternative>& alternatives,
                   const std::string& handles, const GraphType& model, const std::string& children) {
    const bool indexedNames = model.nameIndex;
    const bool deferredIndex = indexedNames || (model.keyComponentIndex && model.serializable);
    const bool tagged = !alternatives.empty();
    const bool hierarchy = !rows.empty() || !children.empty();
    std::string templateParams, componentParams, forwarded;
    const auto append = [](std::string& list, const std::string& item) {
        if (!list.empty()) list += ", ";
        list += item;
    };
    const auto typeParam = [&](std::string type) { return type == vertex ? type + "T" : type; };
    const auto tagType = typeParam("Tag");
    if (tagged) append(templateParams, "class " + tagType);
    std::vector<std::string> componentArgs;
    const auto tupleParam = [&](const std::string& type, const std::string& arg) {
        append(templateParams, "class " + type);
        append(componentParams, type + "&& " + arg);
        auto forward = "std::forward<" + type + ">(" + arg + ")";
        append(forwarded, forward);
        return forward;
    };
    for (size_t i = 0; i < components.size(); ++i)
        componentArgs.push_back(tupleParam(typeParam("Args" + std::to_string(i)), "args" + components[i].publicName));
    const auto payloadArgs = tagged ? tupleParam(typeParam("PayloadArgs"), "payloadArgs") : "";
    out << "\n    template<" << templateParams << ">\n    " << vertex << " emplace_vertex(";
    if (tagged) out << "const " << tagType << "& tag, ";
    out << componentParams << (hierarchy ? ", " + vertex + " parent = null_vertex()" : "") << ") {\n"
        << "        return appendVertex(" << (tagged ? "&tag, " : "") << forwarded
        << (deferredIndex ? ", true" : "") << (hierarchy ? ", parent" : "") << ");\n    }\n";

    for (const auto& alt : alternatives)
        impl << "    auto& payloadColumn(const " << alt.tag << "*) noexcept { return this->" << alt.member << "; }\n";
    impl << "\n    template<" << templateParams << ">\n    " << vertex << " appendVertex(";
    if (tagged) impl << "const " << tagType << "* tag, ";
    impl << componentParams << (deferredIndex ? ", bool indexName" : "") << (hierarchy ? ", " + vertex + " parent" : "") << ") {\n";
    const auto count = "this->" + storage.front() + ".size()";
    const auto descriptor = name + "::" + vertex;
    if (hierarchy)
        impl << "        if (parent != null_vertex() && static_cast<std::uintmax_t>(parent) >= " << count << ")\n"
             << "            throw std::out_of_range(\"invalid " << name << " parent\");\n";
    impl << "        if (" << count << " >= (std::numeric_limits<" << descriptor << ">::max)())\n"
         << "            throw std::length_error(\"" << name << " vertex limit exceeded\");\n"
         << "        const auto vertex = static_cast<" << descriptor << ">(" << count << ");\n";
    // Consume scalar references and vector<bool> proxies before growing any vector columns.
    for (size_t i = 0; i < types.size(); ++i)
        if (!dequeColumns.contains(storage[i]))
            impl << "        auto component" << i << " = std::make_from_tuple<" << types[i] << ">(" << componentArgs[i] << ");\n";
    if (tagged) {
        impl << "        auto& payloads = payloadColumn(tag);\n"
             << "        const auto payloadIndex = static_cast<" << descriptor << ">(payloads.size());\n"
             << "        constexpr auto payloadKind = kindOf(static_cast<" << tagType << "*>(nullptr));\n";
    }
    if (indexedNames) impl << "        auto entry = this->nameIndex_.end();\n";
    if (!rows.empty()) impl << "        auto& edges = this->" << rows << ";\n";
    if (!children.empty()) impl << "        bool linked = false;\n";
    impl << "        try {\n";
    const auto emplaceTuple = [&](const std::string& column, const std::string& tuple, std::string_view indent) {
        impl << indent << "std::apply([&](auto&&... args) {\n"
             << indent << "    " << column << ".emplace_back(std::forward<decltype(args)>(args)...);\n"
             << indent << "}, " << tuple << ");\n";
    };
    if (!input.empty()) {
        const auto i = static_cast<size_t>(std::ranges::find(storage, input) - storage.begin());
        emplaceTuple("this->" + input, componentArgs[i], "            ");
        if (indexedNames)
            impl << "            if (indexName) {\n"
                 << "                const auto inserted = this->nameIndex_.emplace(this->" << input << ".back(), vertex);\n"
                 << "                if (!inserted.second) throw std::invalid_argument(\"duplicate vertex name\");\n"
                 << "                entry = inserted.first;\n            }\n";
    }
    for (size_t i = 0; i < types.size(); ++i)
        if (storage[i] != input && dequeColumns.contains(storage[i]))
            emplaceTuple("this->" + storage[i], componentArgs[i], "            ");
    for (size_t i = types.size(); i < storage.size(); ++i) {
        const auto& field = storage[i];
        if (field != rows && field != handles)
            impl << "            this->" << field << ".emplace_back();\n";
    }
    if (!rows.empty()) impl << "            edges.push_back(VertexLinks{parent});\n";
    if (!children.empty()) impl << "            if (parent != null_vertex()) linked = add_reference(parent, vertex).second;\n";
    if (tagged) {
        impl << "            this->" << handles << ".push_back({payloadKind, payloadIndex});\n";
        std::string boolKinds;
        for (const auto& alt : alternatives) {
            if (!alt.isBoolean) continue;
            if (!boolKinds.empty()) boolKinds += " || ";
            boolKinds += "payloadKind == Kind::" + alt.kind;
        }
        if (boolKinds.empty()) {
            emplaceTuple("payloads", payloadArgs, "            ");
        } else {
            impl << "            if constexpr (" << boolKinds << ") {\n"
                 << "                payloads.push_back({std::make_from_tuple<bool>(" << payloadArgs << ")});\n"
                 << "            } else {\n";
            emplaceTuple("payloads", payloadArgs, "                ");
            impl << "            }\n";
        }
    }
    for (size_t i = 0; i < types.size(); ++i)
        if (storage[i] != input && !dequeColumns.contains(storage[i]))
            impl << "            this->" << storage[i] << ".emplace_back(std::move(component" << i << "));\n";
    if (model.keyComponentIndex) {
        impl << "            if (" << (deferredIndex ? "indexName && " : "")
             << "!this->componentIndex_.emplace(this->" << storage[*model.keyComponentIndex] << ".back(), vertex).second)\n"
             << "                throw std::invalid_argument(\"duplicate vertex key\");\n";
    }
    impl << "        } catch (...) {\n";
    if (indexedNames) impl << "            if (entry != this->nameIndex_.end()) this->nameIndex_.erase(entry);\n";
    if (!children.empty()) impl << "            if (linked) this->" << children << "[parent].pop_back();\n";
    if (tagged) impl << "            if (payloads.size() > payloadIndex) payloads.pop_back();\n";
    for (const auto& field : storage)
        impl << "            if (this->" << field << ".size() > vertex) this->" << field << ".pop_back();\n";
    impl << "            throw;\n        }\n";
    if (!rows.empty()) {
        impl << R"(        if (parent != null_vertex()) {
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
    impl << "        return vertex;\n    }\n";
}

} // namespace

bool hasMoveOnlyValue(const TypeGraph& graph, TypeId id) {
    std::set<uint32_t> visited;
    return hasMoveOnlyValue(graph, id, visited);
}

std::string emitCppGraph(const TypeGraph& graph, TypeId id, std::ostream& out, std::ostream& source,
                         std::set<std::string>& headers, std::set<const TypeNode*>& forwards,
                         const std::vector<BoundField>& fields, const std::string& methods, const std::string& inputArchive,
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
    std::set<std::string> moveOnlyColumns;
    std::set<std::string> dequeColumns;
    const auto columnValue = [&](TypeId type) -> const TypeNode& {
        while (const auto* aliasType = std::get_if<AliasType>(&graph.node(type).data)) type = aliasType->target;
        return graph.node(type);
    };
    const auto columnType = [&](const ComponentInfo& component) {
        if (hasMoveOnlyValue(graph, component.valueType)) moveOnlyColumns.insert(component.name);
        const auto& valueNode = columnValue(component.valueType);
        const auto& value = valueNode.data;
        if (!model.indexedStorage && !std::holds_alternative<BuiltinType>(value) && !std::holds_alternative<EnumType>(value) &&
            !std::holds_alternative<PointerType>(value)) {
            // Segmented columns append without relocating allocator-aware or move-only elements.
            dequeColumns.insert(component.name);
            headers.insert("cocos/base/std/container/deque.h");
        }
        const auto& tag = graph.node(component.tag);
        if (tag.module != node.module && !tag.cppHeader.empty()) {
            headers.insert(tag.cppHeader);
        } else if (component.tag.value > id.value || tag.module != node.module) {
            forwards.insert(&tag);
        }
        return formatTypeName(graph, component.valueType, [&](const TypeNode& type) {
            collectCppHeader(graph, graph.modules.at(node.module.value), type, headers);
            return type.qualifiedName;
        });
    };
    std::vector<std::string> storage;
    std::vector<std::string> types;
    std::set<std::string> members{vertex, name};
    for (const auto& component : model.components) {
        storage.push_back(component.name);
        members.insert(component.name);
        types.push_back(columnType(component));
    }
    const auto keyColumn = model.keyComponentIndex ? storage.at(*model.keyComponentIndex) : "";
    const auto keyType = model.keyComponentIndex ? types.at(*model.keyComponentIndex) : "";
    dequeColumns.erase(keyColumn);
    std::vector<Alternative> alternatives;
    for (const auto& alt : model.alternatives) {
        const auto& tag = graph.node(alt.tag);
        const auto& payload = columnValue(alt.valueType);
        if (payload.qualifiedName == "void" || std::holds_alternative<InterfaceType>(payload.data))
            throw std::runtime_error(node.qualifiedName + "." + alt.name + ": polymorphic storage requires an object type");
        const bool isBoolean = payload.qualifiedName == "bool";
        alternatives.push_back({"::" + tag.qualifiedName, alt.publicName, columnType(alt), alt.name, isBoolean});
        members.insert(alt.name);
    }
    std::set<std::string> api{"allocator_type", "get_allocator", "swapStorage"};
    if (!alternatives.empty()) {
        for (const auto* symbol : {"Kind", "kind", "holds", "value", "visit_object", "kindOf", "VertexHandle"}) {
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
    if (model.vertexList) api.insert({"vertices", "num_vertices", "VertexIterator", "VertexRange"});
    if (model.indexedStorage) api.insert("reserve");
    if (model.incidence) {
        std::set<std::string> symbols{"Edge", "null_vertex", "source", "target", "out_edges", "out_degree",
                                      "adjacent_vertices", "edges", "num_edges", "edge"};
        if (model.bidirectional) symbols.insert({"in_edges", "in_degree", "degree"});
        if (model.mutableGraph) symbols.insert({"add_edge", "remove_edge"});
        for (const auto& symbol : symbols) {
            if (members.contains(symbol)) throw std::runtime_error(node.qualifiedName + ": edge API collides with " + symbol);
        }
        api.insert(symbols.begin(), symbols.end());
    }
    if (model.mutableGraph) {
        std::set<std::string> symbols{"add_vertex", "emplace_vertex", "appendVertex"};
        if (model.removeVertex) symbols.insert("remove_vertex");
        if (!alternatives.empty()) symbols.insert("payloadColumn");
        for (const auto& symbol : symbols)
            if (members.contains(symbol)) throw std::runtime_error(node.qualifiedName + ": vertex API collides with " + symbol);
        api.insert(symbols.begin(), symbols.end());
        if (model.hierarchy && !model.separateEdges) api.insert({"add_edge", "remove_edge"});
    }
    if (model.separateEdges) {
        std::set<std::string> symbols{"null_vertex", "Link", "get_parent", "num_children", "child_links", "child", "parent",
                                      "parent_links", "num_parents", "reference", "has_link"};
        if (model.mutableGraph) symbols.insert({"add_reference", "remove_reference"});
        for (const auto& symbol : symbols)
            if (members.contains(symbol)) throw std::runtime_error(node.qualifiedName + ": reference API collides with " + symbol);
        api.insert(symbols.begin(), symbols.end());
    } else if (model.hierarchy) api.insert({"null_vertex", "VertexLinks", "Link", "ChildIterator", "IteratorRange", "ChildLinkIterator",
        "ChildLinkRange", "ParentLinkRange", "ParentLinkIterator", "get_parent", "num_children", "child_links", "child", "parent", "parent_links",
        "num_parents", "Edge", "source", "target", "out_edges", "out_degree", "in_edges", "in_degree", "degree",
        "adjacent_vertices", "num_edges", "edge", "has_link", "EdgeIterator", "edges"});
    if (model.parentGraph) api.insert("locate_child");
    if (model.addressableGraph) api.insert({"locate", "path", "exists"});
    for (const auto& c : model.components) {
        api.insert("get");
        api.insert("get" + c.publicName);
    }
    for (const auto& f : model.fields) {
        if (hasMoveOnlyValue(graph, f.type.type)) moveOnlyColumns.insert(f.name);
        if (api.contains(f.name) || !members.insert(f.name).second)
            throw std::runtime_error(node.qualifiedName + ": graph field collides with C++ member " + f.name);
    }
    for (const auto& method : model.methods) {
        if (api.contains(method.name) || method.name == name)
            throw std::runtime_error(method.result.location.describe() + ": graph method collides with C++ member " + method.name);
        members.insert(method.name);
    }
    if (model.indexedStorage) {
        const auto base = formatTypeName(graph, *model.indexedStorage, [&](const TypeNode& part) {
            collectCppHeader(graph, graph.modules.at(node.module.value), part, headers);
            return part.qualifiedName;
        });
        out << '\n';
        writeDocumentation(out, node.documentation);
        out << "struct " << name << " : public " << base << " {\n"
            << "    using " << vertex << " = " << integer.qualifiedName << ";\n"
            << "    void reserve(" << vertex << " capacity) {\n        " << base << "::reserve(capacity);\n";
        for (const auto& column : storage) out << "        this->" << column << ".reserve(capacity);\n";
        out << "    }\n";
        componentAccessors(out, graph, model, vertex, storage, types, "std::vector");
        if (!methods.empty()) out << '\n' << methods;
        for (size_t i = 0; i < storage.size(); ++i)
            out << "    std::vector<" << types[i] << "> " << storage[i] << ";\n";
        for (const auto& field : fields) {
            writeDocumentation(out, field.documentation, "    ");
            out << "    " << field.cpp << " " << field.name;
            if (!field.cppDefault.empty()) out << "{" << field.cppDefault << "}";
            out << ";\n";
        }
        out << "};\n";
        return {};
    }
    const auto internalColumn = [&](std::string field) {
        while (!members.insert(field).second) field += "0";
        storage.push_back(field);
        return field;
    };
    const std::string rows = model.hierarchy && !model.separateEdges ? internalColumn("links_") : "";
    const auto adjacency = [&](const char* base) {
        auto column = internalColumn(base);
        dequeColumns.insert(column);
        return column;
    };
    const auto outgoing = model.incidence ? adjacency("outgoing_") : "";
    const auto incoming = model.bidirectional ? adjacency("incoming_") : "";
    const auto childRows = model.separateEdges ? adjacency("children_") : "";
    const auto parentRows = model.separateEdges ? adjacency("parents_") : "";
    const std::string handles = alternatives.empty() ? "" : internalColumn("handles_");
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
        << "    using " << vertex << " = " << integer.qualifiedName << ";\n"
        << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n\n";
    const bool transactionalArchive = transactionalLoad && !inputArchive.empty();
    graphConstructors(out, source, name, allStorage, moveOnlyColumns, fields, model, transactionalArchive);
    if (model.vertexList) {
        headers.insert("cstddef");
        headers.insert("iterator");
        vertexRange(out, vertex);
        out << "    " << vertex << " num_vertices() const noexcept { return static_cast<" << vertex << ">(this->" << storage.front() << ".size()); }\n";
    }
    std::string input;
    std::ostringstream insertion;
    if (model.mutableGraph) {
        headers.insert("limits");
        headers.insert("stdexcept");
        if (model.nameComponentIndex) input = storage.at(*model.nameComponentIndex);
        else if (!types.empty() && types.front() == "ccstd::pmr::string") input = storage.front();
        if (!input.empty()) headers.insert("string_view");
        headers.insert({"tuple", "utility"});
        addVertex(out, source, name, vertex, model.components, input, alternatives, model.hierarchy, keyColumn, keyType);
        emplaceVertex(out, insertion, name, vertex, model.components, storage, types, dequeColumns, input, rows,
                      alternatives, handles, model, childRows);
    }
    if (model.hierarchy || model.incidence || model.nameIndex || model.keyComponentIndex)
        out << "\n    static constexpr " << vertex << " null_vertex() noexcept { return (std::numeric_limits<"
            << vertex << ">::max)(); }\n";
    if (!rows.empty()) {
        for (const auto* symbol : {"Edge", "source", "target", "out_edges", "out_degree", "edge", "ChildIterator", "IteratorRange",
                                   "ParentLinkRange", "ParentLinkIterator", "parent", "parent_links", "num_parents", "in_edges", "in_degree", "degree",
                                   "adjacent_vertices", "num_edges", "has_link", "EdgeIterator", "edges"}) {
            if (members.contains(symbol))
                throw std::runtime_error(node.qualifiedName + ": shared-edge graph member name collision: " + symbol);
        }
        headers.insert({"cstddef", "cstdint", "iterator", "limits", "type_traits", "utility"});
        hierarchyInterface(out, name, vertex, rows);
        if (model.mutableGraph) {
            if (members.contains("add_edge") || members.contains("remove_edge"))
                throw std::runtime_error(node.qualifiedName + ": mutable graph member name collision");
            hierarchyMutation(out, source, name, vertex, rows, model.serializable);
        }
    }
    if (model.incidence || model.separateEdges) {
        headers.insert({"algorithm", "limits", "cocos/base/GraphEdges.h", "cocos/base/std/container/deque.h"});
        if (model.incidence) directedEdges(out, source, name, vertex, outgoing, incoming, model.mutableGraph, false);
        if (model.separateEdges) directedEdges(out, source, name, vertex, childRows, parentRows, model.mutableGraph, true);
    }
    if (model.parentGraph) {
        headers.insert("string_view");
        const auto& selected = model.components.at(model.nameComponentIndex.value());
        const auto nameAccessor = "get" + selected.publicName;
        parentLookup(out, source, name, vertex, selected.name, nameAccessor);
        if (model.addressableGraph) {
            headers.insert("stdexcept");
            headers.insert("cocos/base/std/container/string.h");
            addressableGraph(out, source, name, vertex, selected.name, nameAccessor);
        }
    }
    if (model.keyComponentIndex) {
        out << "\n    bool contains(" << keyType << " key) const noexcept { return find(key) != null_vertex(); }\n"
            << "    " << vertex << " find(" << keyType << " key) const noexcept {\n"
            << "        const auto it = this->componentIndex_.find(key);\n"
            << "        return it == this->componentIndex_.end() ? null_vertex() : it->second;\n    }\n"
            << "    " << vertex << " vertex(" << keyType << " key) const { return this->componentIndex_.at(key); }\n"
            << "    void reserve(" << vertex << " capacity);\n";
        source << "\nvoid " << name << "::reserve(" << vertex << " capacity) {\n";
        for (const auto& field : storage)
            if (!dequeColumns.contains(field)) source << "    this->" << field << ".reserve(capacity);\n";
        source << "    this->componentIndex_.reserve(capacity);\n}\n";
    }
    if (model.removeVertex) {
        headers.insert({"algorithm", "cstddef", "type_traits"});
        removeVertex(out, source, name, vertex, model, storage, outgoing, incoming);
    }
    if (model.nameIndex) {
        out << "\n    bool contains(std::string_view key) const noexcept { return find(key) != null_vertex(); }\n"
            << "    " << vertex << " find(std::string_view key) const noexcept {\n"
            << "        const auto it = this->nameIndex_.find(key);\n"
            << "        return it == this->nameIndex_.end() ? null_vertex() : it->second;\n    }\n"
            << "    " << vertex << " vertex(std::string_view key) const {\n"
            << "        const auto v = find(key);\n        if (v == null_vertex()) throw std::out_of_range(\"unknown vertex name\");\n"
            << "        return v;\n    }\n";
        if (model.mutableGraph) {
            const auto i = *model.nameComponentIndex;
            out << "    void " << nameSetter << "(" << vertex << " v, std::string_view value);\n";
            source << "\nvoid " << name << "::" << nameSetter << "(" << vertex << " v, std::string_view value) {\n"
                   << "    auto& current = this->" << storage[i] << ".at(v);\n"
                   << "    if (std::string_view{current} == value) return;\n"
                   << "    " << types[i] << " replacement(value, current.get_allocator());\n"
                   << "    const auto old = this->nameIndex_.find(std::string_view{current});\n"
                   << "    if (!this->nameIndex_.emplace(value, v).second) throw std::invalid_argument(\"duplicate vertex name\");\n"
                   << "    current.swap(replacement);\n    this->nameIndex_.erase(old);\n}\n";
        }
    }
    componentAccessors(out, graph, model, vertex, storage, types, "ccstd::pmr::vector");
    if (!alternatives.empty()) {
        headers.insert({"cstdint", "variant", "cocos/base/TemplateUtils.h"});
        payloadAccess(out, alternatives, vertex, handles);
    }
    if (!methods.empty()) out << '\n' << methods;
    out << "\nprivate:\n";
    if (model.compareEqual) out << "    friend bool operator==(const " << name << "&, const " << name << "&) noexcept;\n";
    if (model.compareLess) out << "    friend bool operator<(const " << name << "&, const " << name << "&) noexcept;\n";
    if (model.hashable) out << "    friend struct ::ccstd::hash<" << name << ">;\n";
    for (size_t i = 0; i < types.size(); ++i)
        out << "    ccstd::pmr::" << (dequeColumns.contains(storage[i]) ? "deque" : "vector") << "<" << types[i] << "> " << storage[i] << ";\n";
    for (const auto& column : {outgoing, incoming, childRows, parentRows}) {
        if (!column.empty()) out << "    ccstd::pmr::deque<ccstd::pmr::vector<" << vertex << ">> " << column << ";\n";
    }
    if (!rows.empty()) {
        out << "    struct VertexLinks {\n"
            << "        " << vertex << " parent{null_vertex()};\n"
            << "        " << vertex << " firstChild{null_vertex()};\n"
            << "        " << vertex << " lastChild{null_vertex()};\n"
            << "        " << vertex << " nextSibling{null_vertex()};\n"
            << "        " << vertex << " childCount{};\n    };\n"
            << "    ccstd::pmr::vector<VertexLinks> " << rows << ";\n";
    }
    if (!alternatives.empty()) {
        out << "    struct VertexHandle { Kind kind; " << vertex << " index; };\n"
            << "    ccstd::pmr::vector<VertexHandle> " << handles << ";\n";
        for (const auto& alt : alternatives) {
            out << "    static constexpr Kind kindOf(" << alt.tag << "*) noexcept { return Kind::" << alt.kind << "; }\n";
            // Keep value() addressable even when the C++ container specializes vector<bool>.
            if (alt.isBoolean) headers.insert("array");
            out << "    ccstd::pmr::" << (dequeColumns.contains(alt.member) ? "deque" : "vector") << "<" << (alt.isBoolean ? "std::array<bool, 1>" : alt.type)
                << "> " << alt.member << ";\n";
        }
    }
    if (model.nameIndex)
        out << "    ccstd::pmr::map<ccstd::pmr::string, " << vertex << ", std::less<>> nameIndex_;\n";
    if (model.keyComponentIndex)
        out << "    ccstd::pmr::unordered_map<" << keyType << ", " << vertex << "> componentIndex_;\n";
    out << insertion.str();
    if (transactionalArchive || (!model.immovable && (allStorage.size() > 1 || !moveOnlyColumns.empty() || !fields.empty()))) {
        const bool skipTransients = transactionalArchive && std::ranges::any_of(fields, &BoundField::transient);
        out << "    void swapStorage(" << name << "& other"
            << (skipTransients ? ", bool includeTransient = true" : "")
            << ")" << (fields.empty() ? " noexcept" : "") << ";\n";
    }
    if (!inputArchive.empty()) out << "    friend void load(" << inputArchive << "&, " << name << "&);\n";
    if (!fields.empty()) out << "\npublic:\n";
    for (const auto& f : fields) {
        writeDocumentation(out, f.documentation, "    ");
        out << "    " << f.cpp << " " << f.name << (f.cppDefault.empty() ? "" : "{" + f.cppDefault + "}") << ";\n";
    }
    out << "};\n";
    if (inputArchive.empty() || transactionalLoad) return {};
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
