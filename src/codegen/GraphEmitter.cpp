#include "codegen/GraphEmitter.h"
#include "model/TypeGraphBuilder.h"
#include "model/TypeName.h"

#include <algorithm>
#include <map>
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
                       const std::set<std::string>& dequeColumns, const std::vector<BoundField>& fields, bool immovable) {
    const auto& member = storage.front();
    const bool copyable = moveOnlyColumns.empty();
    out << "    explicit " << name << "(const allocator_type& alloc)";
    for (const auto& field : storage)
        out << (field == member ? " : " : ", ") << field << (dequeColumns.contains(field) ? "(alloc)" : "(0, alloc)");
    for (const auto& f : fields)
        if (f.usesAllocator) out << ", " << f.name << "(" << f.allocatorArguments() << ")";
    out << " {}\n";
    if (immovable) {
        out << "    " << name << "(const " << name << "&) = delete;\n"
            << "    " << name << "(" << name << "&&) = delete;\n"
            << "    " << name << "& operator=(const " << name << "&) = delete;\n"
            << "    " << name << "& operator=(" << name << "&&) = delete;\n"
            << "    allocator_type get_allocator() const noexcept { return {this->" << member << ".get_allocator().resource()}; }\n";
        return;
    }
    out << "    " << name << "(const " << name << "& rhs, const allocator_type& alloc)" << (copyable ? ";\n" : " = delete;\n")
        << "    " << name << "(" << name << "&& rhs, const allocator_type& alloc);\n"
        << "    " << name << "(" << name << "&& rhs) : "
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
            source << "        this->" << f.name << " = std::move(rhs." << f.name << ");\n";
        }
    }
    for (const auto& field : storage) source << "        rhs." << field << ".clear();\n";
    const bool transients = std::ranges::any_of(fields, &BoundField::transient);
    source << "    }\n}\n\nvoid " << name << "::swapStorage(" << name << "& other"
           << (transients ? ", bool includeTransient" : "") << ")" << (fields.empty() ? " noexcept" : "") << " {\n";
    for (const auto& field : storage) source << "    this->" << field << ".swap(other." << field << ");\n";
    if (!fields.empty()) source << "    using std::swap;\n";
    for (const auto& f : fields)
        source << "    " << (f.transient ? "if (includeTransient) " : "") << "swap(this->" << f.name << ", other." << f.name << ");\n";
    source << "}\n";
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
    const auto fieldMoves = [&](const FieldInfo& f) { return hasMoveOnlyValue(graph, f.type.type, visited); };
    if (const auto* record = std::get_if<RecordType>(&node.data))
        return record->moveOnly || std::ranges::any_of(record->fields, fieldMoves);
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
                << "        return this->" << alt.member << "[this->" << handles << "[v].index]"
                << (alt.type == "bool" ? "[0]" : "") << ";\n    }\n";
        }
    }
}

void addVertex(std::ostream& out, std::ostream& source, const std::string& name, const std::string& vertex,
               const std::vector<std::string>& storage, const std::string& input, const std::string& rows,
               const std::vector<Alternative>& alternatives, const std::string& handles,
               const std::map<std::string, std::string>& defaultArgs) {
    const bool hierarchy = !rows.empty();
    const auto& member = storage.front();
    const std::string value = input.empty() ? "" : "std::string_view value";
    out << "\n    " << vertex << " add_vertex(" << value;
    if (!value.empty()) out << " = {}";
    if (hierarchy) {
        out << ") { return add_vertex(" << (value.empty() ? "" : "value, ") << "null_vertex()); }\n"
            << "    " << vertex << " add_vertex(" << value << (value.empty() ? "" : ", ") << vertex << " parent";
    }
    if (alternatives.empty()) {
        out << ");\n";
        source << "\n" << name << "::" << vertex << " " << name << "::add_vertex(" << value;
    } else {
        const std::string args = (input.empty() ? "" : ", value") + std::string(hierarchy ? ", parent" : "");
        out << ") { return addVertexImpl(Kind::" << alternatives.front().kind << args << "); }\n";
        for (const auto& alt : alternatives) {
            out << "    " << vertex << " add_vertex(const " << alt.tag << "&";
            if (!input.empty()) out << ", " << value;
            if (hierarchy) out << ", " << vertex << " parent = null_vertex()";
            out << ") { return addVertexImpl(Kind::" << alt.kind << args << "); }\n";
        }
        source << "\n" << name << "::" << vertex << " " << name << "::addVertexImpl(Kind kind"
               << (value.empty() ? "" : ", ") << value;
    }
    if (hierarchy) source << (value.empty() && alternatives.empty() ? "" : ", ") << vertex << " parent";
    source << ") {\n";
    if (hierarchy) {
        source << "    if (parent != null_vertex() && static_cast<std::uintmax_t>(parent) >= this->" << member << ".size())\n"
               << "        throw std::out_of_range(\"invalid " << name << " parent\");\n";
    }
    source << "    if (this->" << member << ".size() >= (std::numeric_limits<" << name << "::" << vertex << ">::max)())\n"
           << "        throw std::length_error(\"" << name << " vertex limit exceeded\");\n"
           << "    const auto vertex = static_cast<" << name << "::" << vertex << ">(this->" << member << ".size());\n";
    const bool parallel = storage.size() > 1 || !alternatives.empty();
    const auto indent = parallel ? "        " : "    ";
    if (hierarchy) source << "    auto& edges = this->" << rows << ";\n";
    if (parallel) source << "    try {\n";
    // Consume the view before growing other columns: it can refer to text in this graph.
    if (!input.empty()) source << indent << "this->" << input << ".emplace_back(value);\n";
    for (const auto& field : storage) {
        if (field != input && field != rows && field != handles)
            source << indent << "this->" << field << ".emplace_back(" << defaultArgs.at(field) << ");\n";
    }
    if (hierarchy) source << indent << "edges.push_back(VertexLinks{parent});\n";
    if (!alternatives.empty()) {
        source << indent << "switch (kind) {\n";
        for (const auto& alt : alternatives) {
            source << indent << "case Kind::" << alt.kind << ":\n"
                   << indent << "    this->" << handles << ".push_back({kind, static_cast<" << vertex
                   << ">(this->" << alt.member << ".size())});\n"
                   << indent << "    this->" << alt.member << ".emplace_back(" << defaultArgs.at(alt.member) << ");\n"
                   << indent << "    break;\n";
        }
        source << indent << "}\n";
    }
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
                  std::set<std::string>& headers, std::set<std::string>& forwards,
                  const std::vector<BoundField>& fields, const std::string& inputArchive) {
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
    headers.insert("cocos/base/std/container/vector.h");
    headers.insert("utility");
    if (!integer.cppHeader.empty()) headers.insert(integer.cppHeader);
    std::set<std::string> moveOnlyColumns;
    std::set<std::string> dequeColumns;
    std::map<std::string, std::string> defaultArgs;
    const auto columnType = [&](const ComponentInfo& component) {
        std::set<uint32_t> visited;
        if (hasMoveOnlyValue(graph, component.valueType, visited)) moveOnlyColumns.insert(component.name);
        const auto& valueNode = graph.node(component.valueType);
        const auto& value = valueNode.data;
        auto& args = defaultArgs[component.name];
        if (valueNode.qualifiedName == "ccstd::pmr::string") args = "\"\"";
        if (const auto* instance = std::get_if<TemplateInstanceType>(&value);
            instance && graph.node(instance->declaration).qualifiedName == "ccstd::pmr::vector") args = "0U";
        if (!std::holds_alternative<BuiltinType>(value) && !std::holds_alternative<EnumType>(value) &&
            !std::holds_alternative<AliasType>(value) && !std::holds_alternative<PointerType>(value)) {
            // Segmented columns append without relocating allocator-aware or move-only elements.
            dequeColumns.insert(component.name);
            headers.insert("cocos/base/std/container/deque.h");
        }
        const auto& tag = graph.node(component.tag);
        if (tag.module != node.module && !tag.cppHeader.empty()) {
            headers.insert(tag.cppHeader);
        } else if (component.tag.value > id.value || tag.module != node.module) {
            const auto ns = tag.qualifiedName.rfind("::");
            const auto declaration = "struct " + tag.name + ";";
            forwards.insert(ns == std::string::npos ? declaration : "namespace " + tag.qualifiedName.substr(0, ns) + " { " + declaration + " }");
        }
        return formatTypeName(graph, component.valueType, [&](const TypeNode& type) {
            if (!type.cppHeader.empty()) headers.insert(type.cppHeader);
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
    std::vector<Alternative> alternatives;
    for (const auto& alt : model.alternatives) {
        const auto& tag = graph.node(alt.tag);
        const auto& payload = graph.node(alt.valueType);
        if (payload.qualifiedName == "void" || std::holds_alternative<InterfaceType>(payload.data))
            throw std::runtime_error(node.qualifiedName + "." + alt.name + ": polymorphic storage requires an object type");
        alternatives.push_back({"::" + tag.qualifiedName, componentAccessorSuffix(tag.name), columnType(alt), alt.name});
        members.insert(alt.name);
    }
    if (!alternatives.empty()) {
        for (const auto* symbol : {"Kind", "kind", "holds", "value", "kindOf", "VertexHandle", "addVertexImpl"}) {
            if (members.contains(symbol))
                throw std::runtime_error(node.qualifiedName + ": polymorphic graph member name collision: " + symbol);
        }
    }
    std::set<std::string> api{"allocator_type", "get_allocator", "swapStorage"};
    if (model.vertexList) api.insert({"vertices", "num_vertices", "VertexIterator", "VertexRange"});
    if (model.mutableGraph) {
        api.insert("add_vertex");
        if (model.hierarchy) api.insert({"add_edge", "remove_edge"});
    }
    if (model.hierarchy) api.insert({"null_vertex", "VertexLinks", "Link", "ChildIterator", "IteratorRange", "ChildLinkIterator",
        "ChildLinkRange", "ParentLinkRange", "ParentLinkIterator", "get_parent", "num_children", "child_links", "child", "parent", "parent_links",
        "num_parents", "Edge", "source", "target", "out_edges", "out_degree", "in_edges", "in_degree", "degree",
        "adjacent_vertices", "num_edges", "edge", "has_link", "EdgeIterator", "edges"});
    if (model.parentGraph) api.insert("locate_child");
    if (model.addressableGraph) api.insert({"locate", "path", "exists"});
    if (!alternatives.empty()) api.insert({"Kind", "kind", "holds", "value", "kindOf", "VertexHandle", "addVertexImpl"});
    for (const auto& c : model.components) {
        api.insert("get");
        api.insert("get" + componentAccessorSuffix(graph.node(c.tag).name));
    }
    for (const auto& f : model.fields) {
        std::set<uint32_t> visited;
        if (hasMoveOnlyValue(graph, f.type.type, visited)) moveOnlyColumns.insert(f.name);
        if (api.contains(f.name) || !members.insert(f.name).second)
            throw std::runtime_error(node.qualifiedName + ": graph field collides with C++ member " + f.name);
    }
    const auto internalColumn = [&](std::string field) {
        while (!members.insert(field).second) field += "0";
        storage.push_back(field);
        return field;
    };
    const std::string rows = model.hierarchy ? internalColumn("links_") : "";
    const std::string handles = alternatives.empty() ? "" : internalColumn("handles_");
    // Only these columns have one entry per vertex. Each payload column grows for its own kind.
    auto allStorage = storage;
    for (const auto& alt : alternatives) allStorage.push_back(alt.member);

    out << "\nstruct " << name << " {\n"
        << "    using " << vertex << " = " << integer.qualifiedName << ";\n"
        << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n\n";
    graphConstructors(out, source, name, allStorage, moveOnlyColumns, dequeColumns, fields, model.immovable);
    if (model.vertexList) {
        headers.insert("cstddef");
        headers.insert("iterator");
        vertexRange(out, vertex);
        out << "    " << vertex << " num_vertices() const noexcept { return static_cast<" << vertex << ">(this->" << storage.front() << ".size()); }\n";
    }
    std::string input;
    if (model.mutableGraph) {
        headers.insert("limits");
        headers.insert("stdexcept");
        if (model.nameComponentIndex) input = storage.at(*model.nameComponentIndex);
        else if (!types.empty() && types.front() == "ccstd::pmr::string") input = storage.front();
        if (!input.empty()) headers.insert("string_view");
        addVertex(out, source, name, vertex, storage, input, rows, alternatives, handles, defaultArgs);
    }
    if (model.hierarchy) {
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
    if (!alternatives.empty()) {
        headers.insert({"cstdint", "variant"});
        payloadAccess(out, alternatives, vertex, handles);
    }
    out << "\nprivate:\n";
    for (size_t i = 0; i < types.size(); ++i)
        out << "    ccstd::pmr::" << (dequeColumns.contains(storage[i]) ? "deque" : "vector") << "<" << types[i] << "> " << storage[i] << ";\n";
    if (model.hierarchy) {
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
            if (alt.type == "bool") headers.insert("array");
            out << "    ccstd::pmr::" << (dequeColumns.contains(alt.member) ? "deque" : "vector") << "<" << (alt.type == "bool" ? "std::array<bool, 1>" : alt.type)
                << "> " << alt.member << ";\n";
        }
        if (model.mutableGraph) {
            out << "    " << vertex << " addVertexImpl(Kind kind";
            if (!input.empty()) out << ", std::string_view value";
            if (model.hierarchy) out << ", " << vertex << " parent";
            out << ");\n";
        }
    }
    if (!model.immovable && (allStorage.size() > 1 || !moveOnlyColumns.empty() || !fields.empty()))
        out << "    void swapStorage(" << name << "& other"
            << (std::ranges::any_of(fields, &BoundField::transient) ? ", bool includeTransient = true" : "")
            << ")" << (fields.empty() ? " noexcept" : "") << ";\n";
    if (!inputArchive.empty()) out << "    friend void load(" << inputArchive << "&, " << name << "&);\n";
    if (!fields.empty()) out << "\npublic:\n";
    for (const auto& f : fields)
        out << "    " << f.cpp << " " << f.name << (f.cppDefault.empty() ? "" : "{" + f.cppDefault + "}") << ";\n";
    out << "};\n";
}

} // namespace cc::codegen
