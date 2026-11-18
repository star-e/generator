module CocosTypeGraph;
import CocosModelSupport;

namespace cc {
namespace codegen {
    @comparison("==") struct TypeId { value: uint32_t = ~0U; }
    @comparison("==") struct ModuleId { value: uint32_t = ~0U; }

    struct TypeUse {
        type: TypeId;
        location: SourceLocation;
    }
    struct ParameterInfo {
        type: TypeUse;
        name: std::string;
    }
    struct MethodInfo {
        result: TypeUse;
        name: std::string;
        parameters: std::vector<ParameterInfo>;
        isConst: bool = false;
        isNoexcept: bool = false;
        typescriptName: std::optional<std::string>;
    }
    struct FieldInfo {
        type: TypeUse;
        name: std::string;
        initializer: std::string;
        isReadonly: bool = false;
        isNullable: bool = false;
    }
    struct Enumerator {
        name: std::string;
        initializer: std::string;
        value: std::variant<int64_t, uint64_t>;
    }

    struct NamespaceType {}
    struct TagType {}
    struct BuiltinType {}
    struct ExternalTypeNode { isStruct: bool = false; }
    struct EnumType { underlying: TypeId; values: std::vector<Enumerator>; isFlags: bool = false; }
    struct RecordType {
        constructors: std::vector<std::vector<uint32_t>>;
        bases: std::vector<TypeId>;
        fields: std::vector<FieldInfo>;
        methods: std::vector<MethodInfo>;
    }
    struct InterfaceType { bases: std::vector<TypeId>; methods: std::vector<MethodInfo>; }
    struct ComponentInfo { tag: TypeId; valueType: TypeId; name: std::string; }
    struct GraphType {
        vertexType: TypeId;
        indexedVertices: bool = false;
        vertexList: bool = false;
        incidence: bool = false;
        bidirectional: bool = false;
        mutableGraph: bool = false;
        hierarchy: bool = false;
        components: std::vector<ComponentInfo>;
        fields: std::vector<FieldInfo>;
        methods: std::vector<MethodInfo>;
    }
    struct AliasType { target: TypeId; }
    @comparison("==") struct PointerType { pointee: TypeId; }
    @comparison("==") struct ReferenceType { referred: TypeId; rvalue: bool = false; }
    @comparison("==") struct QualifiedType { base: TypeId; isConst: bool = false; isVolatile: bool = false; }
    @comparison("==") struct FunctionType { result: TypeId; parameters: std::vector<TypeId>; isNoexcept: bool = false; }
    struct TemplateParameterType { owner: TypeId; index: uint32_t = 0; }
    type TemplateArgument = std::variant<TypeId, std::string>;
    @comparison("==") struct TemplateInstanceType { declaration: TypeId; arguments: std::vector<TemplateArgument>; }

    type TypeData = std::variant<NamespaceType, TagType, BuiltinType, ExternalTypeNode, EnumType,
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
        templateParameters: std::vector<TypeId>;
    }
    struct TypeModule {
        name: std::string;
        namespaces: std::vector<std::string>;
        declarations: std::vector<TypeId>;
        imports: std::vector<ModuleId>;
    }

    @tag struct Node_ {}
    graph TypeGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        incidence_graph { out_edges: vector; }
        bidirectional_graph { in_edges: vector; }
        component Node_: TypeNode { member: nodes; }
        modules: std::vector<TypeModule>;
        nameIndex: std::map<std::string, TypeId>;
        // Scope edges run from parent to child. Vertices retain their IDs throughout a build.
        fn appendNode(node: TypeNode, parent: TypeId) -> TypeId;
        fn node(id: TypeId) const -> const TypeNode&;
        fn findType(qualifiedName: const std::string&) const -> TypeId;
        fn makePointer(pointee: TypeId) -> TypeId;
        fn makeReference(referred: TypeId, rvalue: bool) -> TypeId;
        fn makeQualified(base: TypeId, isConst: bool, isVolatile: bool) -> TypeId;
        fn makeFunction(result: TypeId, parameters: std::vector<TypeId>, isNoexcept: bool) -> TypeId;
        fn makeTemplateInstance(declaration: TypeId, arguments: std::vector<TemplateArgument>) -> TypeId;
    }
}
}
