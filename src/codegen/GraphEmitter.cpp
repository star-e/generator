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
        << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n\n"
        << "    explicit " << name << "(const allocator_type& alloc) noexcept : " << member << "(alloc) {}\n"
        << "    " << name << "(const " << name << "& rhs, const allocator_type& alloc);\n"
        << "    " << name << "(" << name << "&& rhs, const allocator_type& alloc);\n"
        << "    " << name << "(" << name << "&&) noexcept = default;\n"
        << "    " << name << "(const " << name << "&) = delete;\n"
        << "    " << name << "& operator=(" << name << "&&) = default;\n"
        << "    " << name << "& operator=(const " << name << "&) = default;\n"
        << "    allocator_type get_allocator() const noexcept { return {this->" << member << ".get_allocator().resource()}; }\n";
    source << "\n" << name << "::" << name << "(const " << name << "& rhs, const allocator_type& alloc)\n"
           << ": " << member << "(rhs." << member << ", alloc) {}\n"
           << "\n" << name << "::" << name << "(" << name << "&& rhs, const allocator_type& alloc)\n"
           << ": " << member << "(std::move(rhs." << member << "), alloc) {}\n";
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
        out << "\n    " << vertex << " add_vertex(std::string_view value = {});\n";
        source << "\n" << name << "::" << vertex << " " << name << "::add_vertex(std::string_view value) {\n"
               << "    if (this->" << member << ".size() >= (std::numeric_limits<" << name << "::" << vertex << ">::max)())\n"
               << "        throw std::length_error(\"" << name << " vertex limit exceeded\");\n"
               << "    const auto vertex = static_cast<" << name << "::" << vertex << ">(this->" << member << ".size());\n"
               << "    this->" << member << ".emplace_back(value);\n    return vertex;\n}\n";
    }
    for (const auto qualifier : {std::string_view{}, std::string_view{" const"}}) {
        out << "\n    " << value.qualifiedName << qualifier << "& " << accessor << "(" << vertex << " v)" << qualifier
            << " noexcept { return this->" << member << "[v]; }\n"
            << "    " << value.qualifiedName << qualifier << "& get(const ::" << tag.qualifiedName << "&, " << vertex << " v)"
            << qualifier << " noexcept { return " << accessor << "(v); }\n";
    }
    out << "\nprivate:\n    ccstd::pmr::vector<" << value.qualifiedName << "> " << member << ";\n};\n";
}

} // namespace cc::codegen
