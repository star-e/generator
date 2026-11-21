module TypeGraph;
import HostTypes;
import TypeGraphCore;

namespace cc::codegen {
    struct Enumerator {
        name: std::string;
        initializer: std::string;
        value: std::variant<std::int64_t, uint64_t>;
        documentation: std::string;
    }

    struct EnumType { underlying: TypeId; values: std::vector<Enumerator>; isFlags: bool = false; }
    using TemplateArgument = std::variant<TypeId, std::string>;
    @comparison("==") struct TemplateInstanceType { declaration: TypeId; arguments: std::vector<TemplateArgument>; }

    using TypeData = std::variant<NamespaceType, TagType, BuiltinType, ExternalTypeNode, EnumType,
        RecordType, InterfaceType, GraphType, AliasType, PointerType, ReferenceType,
        QualifiedType, FunctionType, TemplateParameterType, TemplateInstanceType>;
    struct TypeNode {
        name: std::string;
        qualifiedName: std::string;
        module: ModuleId;
        data: TypeData;
        cppHeader: std::string;
        typescriptName: std::optional<std::string>;
        typescriptModule: std::string;
        typescriptArchiveModule: std::string;
        typescriptContainer: ContainerKind = ContainerKind::None;
        typescriptMapKey: MapKeyKind = MapKeyKind::Native;
        typescriptNumber: bool = false;
        typescriptTypeOnly: bool = false;
        cppOnly: bool = false;
        templateParameters: std::vector<TypeId>;
        variadicTemplate: bool = false;
        documentation: std::string;
    }

    graph TypeGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed(graph::BidirectionalStorage<uint32_t>);
        vertex_list_graph;
        incidence_graph { out_edges: vector; }
        bidirectional_graph { in_edges: vector; }
        component Node_: TypeNode { member: nodes; }
        modules: std::vector<TypeModule>;
        nameIndex: std::map<std::string, TypeId>;
        // Scope edges run from parent to child. Vertices retain their IDs throughout a build.
        TypeId appendNode(TypeNode node, TypeId parent);
        const TypeNode& node(TypeId id) const;
        TypeId findType(const std::string& qualifiedName) const;
        TypeId makePointer(TypeId pointee);
        TypeId makeReference(TypeId referred, bool rvalue);
        TypeId makeQualified(TypeId base, bool isConst, bool isVolatile);
        TypeId makeFunction(TypeId result, std::vector<TypeId> parameters, bool isNoexcept);
        TypeId makeTemplateInstance(TypeId declaration, std::vector<TemplateArgument> arguments);
    }
}
