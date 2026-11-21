extern module CocosModelSupport;
@integer(signed=false, bits=32) struct uint32_t;
@integer(signed=true, bits=64) struct int64_t;
@integer(signed=false, bits=64) struct uint64_t;
namespace std {
    @string @comparison("==") struct string;
    @container("array") @comparison("==") template<T> class vector;
    @container("optional") template<T> class optional;
    @container("variant") @comparison("==") template<Ts...> class variant;
    @container("map") template<K,V> class map;
}
namespace cc {
    namespace codegen {
        @comparison("==") @fixed_layout(size=4, alignment=4) struct TypeId;
        @comparison("==") @fixed_layout(size=4, alignment=4) struct ModuleId;
        struct NamespaceType;
        struct TagType;
        struct BuiltinType;
        enum class ContainerKind : uint32_t;
        enum class MapKeyKind : uint32_t;
        struct ExternalTypeNode;
        struct RecordType;
        struct InterfaceType;
        struct GraphType;
        struct AliasType;
        struct PointerType;
        struct ReferenceType;
        struct QualifiedType;
        struct FunctionType;
        struct TemplateParameterType;
        struct TypeModule;
        @tag struct Node_;
    }
}
