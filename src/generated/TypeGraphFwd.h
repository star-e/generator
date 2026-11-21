#pragma once

#include "model/GraphSupport.h" // IWYU pragma: export

namespace cc {

namespace codegen {

struct TypeId;
struct ModuleId;
struct TypeUse;
struct ParameterInfo;
enum class AccessorKind : uint32_t;
enum class ResultPresence : uint32_t;
struct MethodInfo;
struct FieldInfo;
struct Enumerator;
struct NamespaceType;
struct TagType;
struct BuiltinType;
enum class ContainerKind : uint32_t;
enum class MapKeyKind : uint32_t;
struct ExternalTypeNode;
struct EnumType;
struct RecordType;
struct CallbackTypeInfo;
struct CallbackParameterInfo;
struct CallbackInfo;
struct InterfaceType;
struct ComponentInfo;
struct GraphType;
struct AliasType;
struct PointerType;
struct ReferenceType;
struct QualifiedType;
struct FunctionType;
struct TemplateParameterType;
struct TemplateInstanceType;
struct TypeNode;
struct TypeModule;
struct Node_;
struct TypeGraph;

} // namespace codegen

} // namespace cc
