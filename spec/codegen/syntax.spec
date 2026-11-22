module Syntax;
import HostTypes;
import SourceLocation;
import TypeGraph;

namespace cc::codegen {
    struct TypeRef {
        location: SourceLocation;
        name: std::string;
        pointer: bool = false;
        arguments: std::vector<TypeRef>;
        constPointee: bool = false;
        reference: bool = false;
        hasTemplateArguments: bool = false;
    }

    struct Parameter {
        type: TypeRef;
        name: std::string;
        byConstReference: bool = false;
        defaultValue: std::string;
    }

    struct Method {
        result: TypeRef;
        name: std::string;
        parameters: std::vector<Parameter>;
        isConst: bool = false;
        isNoexcept: bool = false;
        cppOnly: bool = false;
        cppBody: std::string;
        isStatic: bool = false;
        isPureVirtual: bool = false;
        isExplicitConversion: bool = false;
        explicitCppOnly: bool = false;
        accessor: AccessorKind = AccessorKind::None;
        optionalMethod: bool = false;
        resultPresence: ResultPresence = ResultPresence::Required;
        cppPrivate: bool = false;
        documentation: std::string;
    }

    struct Field {
        type: TypeRef;
        name: std::string;
        initializer: std::string;
        isReadonly: bool = false;
        isNullable: bool = false;
        transient: bool = false;
        cppOnly: bool = false;
        notElement: bool = false;
        isMutable: bool = false;
        cppPrivate: bool = false;
        documentation: std::string;
    }

    struct Constructor {
        location: SourceLocation;
        fields: std::vector<std::string>;
        defaults: bool = false;
        cppOnly: bool = false;
        custom: bool = false;
        isNoexcept: bool = false;
        cppPrivate: bool = false;
        documentation: std::string;
    }

    enum class ArchiveRole { none, input, output }

    struct Interface {
        methods: std::vector<Method>;
        bases: std::vector<TypeRef>;
        archiveRole: ArchiveRole = ArchiveRole::none;
    }

    struct Record {
        isClass: bool = false;
        isFinal: bool = false;
        bases: std::vector<TypeRef>;
        implements: std::optional<TypeRef>;
        destructorNoexcept: std::optional<bool>;
        destructorPrivate: bool = false;
        cppMembers: std::string;
        moveOnly: bool = false;
        immovable: bool = false;
        serializable: bool = false;
        comparisons: std::set<std::string>;
        hashable: bool = false;
        fields: std::vector<Field>;
        staticFields: std::vector<Field>;
        constructors: std::vector<Constructor>;
        methods: std::vector<Method>;
        destructorDocumentation: std::string;
    }

    struct LeftShift {
        count: std::string;
        location: SourceLocation;
    }

    struct EnumMember {
        name: std::string;
        initializer: std::string;
        initializerLocation: SourceLocation;
        leftShift: std::optional<LeftShift>;
        documentation: std::string;
    }

    struct Enum {
        values: std::vector<EnumMember>;
        underlying: std::optional<TypeRef>;
        isFlags: bool = false;
    }

    struct ExternalDeclaration {
        isStruct: bool = false;
        isString: bool = false;
        isPmr: bool = false;
        constructorConstRef: bool = false;
        container: ContainerKind = ContainerKind::None;
        templateParameters: std::vector<std::string>;
        variadicTemplate: bool = false;
    }

    struct TagDeclaration {}
    struct AliasDeclaration { target: TypeRef; }

    struct GraphComponent {
        tag: TypeRef;
        value: TypeRef;
        member: std::string;
        kindName: std::string;
    }

    struct GraphDeclaration {
        immovable: bool = false;
        serializable: bool = false;
        comparisons: std::set<std::string>;
        hashable: bool = false;
        vertexAlias: std::string;
        vertexType: std::optional<TypeRef>;
        indexedStorage: std::optional<TypeRef>;
        indexed: bool = false;
        vertexList: bool = false;
        mutableGraph: bool = false;
        incidence: bool = false;
        bidirectional: bool = false;
        hierarchy: bool = false;
        separateEdges: bool = false;
        nameComponent: std::optional<TypeRef>;
        nameIndex: std::optional<SourceLocation>;
        indexedComponent: std::optional<TypeRef>;
        removeVertex: bool = false;
        parentGraph: std::optional<SourceLocation>;
        addressableGraph: std::optional<SourceLocation>;
        components: std::vector<GraphComponent>;
        alternatives: std::vector<GraphComponent>;
        fields: std::vector<Field>;
        methods: std::vector<Method>;
    }

    struct Declaration {
        name: std::string;
        location: SourceLocation;
        data: std::variant<Interface, Record, Enum, ExternalDeclaration, TagDeclaration, GraphDeclaration, AliasDeclaration>;
        externalScope: std::string;
        cppHeader: std::string;
        cppOnly: bool = false;
        documentation: std::string;
    }

    struct ModuleImport {
        location: SourceLocation;
        name: std::string;
    }

    struct PoolBinding {
        member: std::string;
        keepOnReset: std::set<std::string>;
        assignFields: bool = false;
    }

    struct PoolDependency {
        member: std::string;
        module: std::string;
    }

    enum class BindingTarget { type, interfaceType, classType, enumeration, constEnum, numberField }

    struct TypeScriptImport {
        name: std::string;
        from: std::filesystem::path;
    }

    struct TypeScriptNamedImport {
        names: std::set<std::string>;
        from: std::string;
        typeOnly: bool = false;
    }

    struct InterfaceMethodBinding {
        target: std::string;
        accessor: AccessorKind = AccessorKind::None;
        nullable: bool = false;
        optional: std::vector<std::string>;
    }

    using MethodBindings = std::map<std::string, InterfaceMethodBinding>;
    using InterfaceBindings = std::map<std::pair<std::string, std::string>, InterfaceMethodBinding>;

    struct CallbackParameter {
        name: std::string;
        optional: bool = false;
        type: TypeRef;
    }

    struct Callback {
        name: std::string;
        optional: bool = false;
        parameters: std::vector<CallbackParameter>;
        result: TypeRef;
        optionalResult: bool = false;
        documentation: std::string;
    }

    struct TypeScriptMethodParameter {
        name: std::string;
        type: std::string;
        optional: bool = false;
    }

    struct TypeScriptClassMethod {
        name: std::string;
        parameters: std::vector<TypeScriptMethodParameter>;
        result: std::string;
        body: std::string;
        documentation: std::string;
    }

    // A raw methods block from an older spec occupies its original position in the class.
    using TypeScriptClassMethods = std::vector<std::variant<TypeScriptClassMethod, std::string>>;

    struct TypeScriptBinding {
        source: TypeRef;
        kind: BindingTarget = BindingTarget::type;
        target: std::string;
        from: std::filesystem::path;
        archiveFrom: std::filesystem::path;
        externalTarget: bool = false;
        externalInterface: bool = false;
        methods: MethodBindings;
        callbacks: std::vector<Callback>;
        renamedFrom: std::map<std::string, std::string>;
        numberField: std::string;
        fieldDefaults: std::map<std::string, std::string>;
        base: std::optional<TypeScriptImport>;
        classMethods: TypeScriptClassMethods;
        pool: std::optional<PoolBinding>;
        constructor: std::optional<std::vector<std::string>>;
        requiredConstructor: bool = false;
        mapKey: MapKeyKind = MapKeyKind::Native;
        names: bool = false;
        registerEnum: bool = false;
        helpers: bool = false;
    }

    struct SwigMember {
        location: SourceLocation;
        owner: std::string;
        name: std::string;
    }

    struct SwigBindings {
        includes: std::vector<std::filesystem::path>;
        macros: std::vector<std::pair<SwigMember, std::string>>;
        releaseInGC: std::vector<SwigMember>;
    }

    struct ModuleBindings {
        typescript: std::vector<TypeScriptBinding>;
        typescriptImports: std::vector<TypeScriptNamedImport>;
        typescriptFrom: std::filesystem::path;
        typescriptExports: std::map<std::string, SourceLocation>;
        poolModule: std::filesystem::path;
        poolDependencies: std::vector<PoolDependency>;
        jsb: std::optional<SourceLocation>;
        jsbRecords: std::map<std::string, SourceLocation>;
        jsbIncludes: std::set<std::filesystem::path>;
        swig: std::optional<SwigBindings>;
    }

    struct Module {
        name: std::string;
        namespaces: std::vector<std::string>;
        declarations: std::vector<Declaration>;
        imports: std::vector<ModuleImport>;
        bindings: ModuleBindings;
        cppHeader: std::string;
        external: bool = false;
        pool: bool = false;
    }
}
