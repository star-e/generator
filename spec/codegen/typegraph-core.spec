module TypeGraphCore;
import SourceLocation;
import HostTypes;

namespace cc::codegen {
    @comparison("==") struct TypeId { value: uint32_t = 0xFFFFFFFF; }
    @comparison("==") struct ModuleId { value: uint32_t = 0xFFFFFFFF; }

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
        explicitCppOnly: bool = false;
        declaredProperty: std::string;
        accessor: AccessorKind = AccessorKind::None;
        optionalMethod: bool = false;
        resultPresence: ResultPresence = ResultPresence::Required;
        cppPrivate: bool = false;
        documentation: std::string;
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
        isMutable: bool = false;
        cppPrivate: bool = false;
        documentation: std::string;
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

    struct ConstructorInfo {
        fields: std::vector<uint32_t>;
        defaults: bool = false;
        cppOnly: bool = false;
        custom: bool = false;
        isNoexcept: bool = false;
        cppPrivate: bool = false;
        documentation: std::string;
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
        implements: std::optional<TypeId>;
        isClass: bool = false;
        isFinal: bool = false;
        destructorNoexcept: std::optional<bool>;
        destructorPrivate: bool = false;
        cppMembers: std::string;
        fields: std::vector<FieldInfo>;
        staticFields: std::vector<FieldInfo>;
        methods: std::vector<MethodInfo>;
        destructorDocumentation: std::string;
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
        documentation: std::string;
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

    struct TypeModule {
        name: std::string;
        namespaces: std::vector<std::string>;
        declarations: std::vector<TypeId>;
        imports: std::vector<ModuleId>;
        inputArchive: TypeId;
        outputArchive: TypeId;
    }

    @tag struct Node_ {}
}
