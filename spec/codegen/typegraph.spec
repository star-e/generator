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
        byConstReference: bool = false;
        defaultValue: std::string;
    }
    enum class AccessorKind : uint32_t { None, Getter, Setter }
    enum class ResultPresence : uint32_t { Required, Nullable, Optional }
    struct MethodInfo {
        result: TypeUse;
        name: std::string;
        parameters: std::vector<ParameterInfo>;
        isConst: bool = false;
        isNoexcept: bool = false;
        cppOnly: bool = false;
        typescriptName: std::optional<std::string>;
        cppBody: std::string;
        isStatic: bool = false;
        isPureVirtual: bool = false;
        isExplicitConversion: bool = false;
        accessor: AccessorKind = AccessorKind::None;
        optionalMethod: bool = false;
        resultPresence: ResultPresence = ResultPresence::Required;
    }
    struct FieldInfo {
        type: TypeUse;
        name: std::string;
        initializer: std::string;
        isReadonly: bool = false;
        isNullable: bool = false;
        transient: bool = false;
        typescriptInitializer: std::string;
        cppOnly: bool = false;
        notElement: bool = false;
    }
    struct Enumerator {
        name: std::string;
        initializer: std::string;
        value: std::variant<int64_t, uint64_t>;
        deprecationMessage: std::optional<std::string>;
    }

    struct NamespaceType {}
    struct TagType {}
    struct BuiltinType {}
    enum class ContainerKind : uint32_t { None, Array, Map, Set }
    enum class MapKeyKind : uint32_t { Native, String, Json }
    struct ExternalTypeNode {
        isStruct: bool = false;
        isString: bool = false;
        isPmr: bool = false;
        constructorConstRef: bool = false;
        container: ContainerKind = ContainerKind::None;
    }
    struct EnumType { underlying: TypeId; values: std::vector<Enumerator>; isFlags: bool = false; }
    struct ConstructorInfo {
        fields: std::vector<uint32_t>;
        defaults: bool = false;
        cppOnly: bool = false;
    }
    struct RecordType {
        moveOnly: bool = false;
        immovable: bool = false;
        serializable: bool = false;
        compareEqual: bool = false;
        compareLess: bool = false;
        compareGreater: bool = false;
        hashable: bool = false;
        constructors: std::vector<ConstructorInfo>;
        typescriptConstructor: std::optional<ConstructorInfo>;
        bases: std::vector<TypeId>;
        fields: std::vector<FieldInfo>;
        staticFields: std::vector<FieldInfo>;
        methods: std::vector<MethodInfo>;
    }
    struct CallbackTypeInfo {
        declaration: TypeUse;
        name: std::string;
        arguments: std::vector<CallbackTypeInfo>;
    }
    struct CallbackParameterInfo {
        name: std::string;
        optional: bool = false;
        type: CallbackTypeInfo;
    }
    struct CallbackInfo {
        name: std::string;
        optional: bool = false;
        parameters: std::vector<CallbackParameterInfo>;
        result: CallbackTypeInfo;
        optionalResult: bool = false;
    }
    struct InterfaceType {
        bases: std::vector<TypeId>;
        methods: std::vector<MethodInfo>;
        typescriptCallbacks: std::vector<CallbackInfo>;
    }
    struct ComponentInfo { tag: TypeId; valueType: TypeId; name: std::string; publicName: std::string; }
    struct GraphType {
        immovable: bool = false;
        serializable: bool = false;
        compareEqual: bool = false;
        compareLess: bool = false;
        compareGreater: bool = false;
        hashable: bool = false;
        vertexType: TypeId;
        indexedVertices: bool = false;
        vertexList: bool = false;
        incidence: bool = false;
        bidirectional: bool = false;
        mutableGraph: bool = false;
        hierarchy: bool = false;
        separateEdges: bool = false;
        nameComponentIndex: std::optional<uint32_t>;
        nameIndex: bool = false;
        keyComponentIndex: std::optional<uint32_t>;
        removeVertex: bool = false;
        parentGraph: bool = false;
        addressableGraph: bool = false;
        components: std::vector<ComponentInfo>;
        alternatives: std::vector<ComponentInfo>;
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
        typescriptArchiveModule: std::string;
        typescriptContainer: ContainerKind = ContainerKind::None;
        typescriptMapKey: MapKeyKind = MapKeyKind::Native;
        typescriptNumber: bool = false;
        typescriptTypeOnly: bool = false;
        cppOnly: bool = false;
        templateParameters: std::vector<TypeId>;
    }
    struct TypeModule {
        name: std::string;
        namespaces: std::vector<std::string>;
        declarations: std::vector<TypeId>;
        imports: std::vector<ModuleId>;
        inputArchive: TypeId;
        outputArchive: TypeId;
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
