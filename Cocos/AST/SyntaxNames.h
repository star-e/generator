#pragma once
#include <Cocos/AST/SyntaxTypes.h>

namespace Cocos {

namespace Meta {

inline const char* getName(const Requires_& v) noexcept { return "Requires"; }
inline const char* getName(const Composites_& v) noexcept { return "Composites"; }
inline const char* getName(const Reuses_& v) noexcept { return "Reuses"; }
inline const char* getName(const Inherits_& v) noexcept { return "Inherits"; }
inline const char* getName(const Sums_& v) noexcept { return "Sums"; }
inline const char* getName(const References_& v) noexcept { return "References"; }
inline const char* getName(const Optional& v) noexcept { return "Optional"; }
inline const char* getName(const Container& v) noexcept { return "Container"; }
inline const char* getName(const Map& v) noexcept { return "Map"; }
inline const char* getName(const Instance& v) noexcept { return "Instance"; }
inline const char* getName(const Namespace& v) noexcept { return "Namespace"; }
inline const char* getName(const Declare& v) noexcept { return "Declare"; }
inline const char* getName(const Alias& v) noexcept { return "Alias"; }
inline const char* getName(const Concept& v) noexcept { return "Concept"; }
inline const char* getName(const Traits& v) noexcept { return "Traits"; }
inline const char* getName(const Value& v) noexcept { return "Value"; }
inline const char* getName(const EnumValue& v) noexcept { return "EnumValue"; }
inline const char* getName(const Enum& v) noexcept { return "Enum"; }
inline const char* getName(const Tag& v) noexcept { return "Tag"; }
inline const char* getName(const Member& v) noexcept { return "Member"; }
inline const char* getName(const Constructor& v) noexcept { return "Constructor"; }
inline const char* getName(const Struct& v) noexcept { return "Struct"; }
inline const char* getName(const Variant& v) noexcept { return "Variant"; }
inline const char* getName(const Namespace_& v) noexcept { return "Namespace"; }
inline const char* getName(const Declare_& v) noexcept { return "Declare"; }
inline const char* getName(const Concept_& v) noexcept { return "Concept"; }
inline const char* getName(const Value_& v) noexcept { return "Value"; }
inline const char* getName(const Enum_& v) noexcept { return "Enum"; }
inline const char* getName(const Tag_& v) noexcept { return "Tag"; }
inline const char* getName(const Struct_& v) noexcept { return "Struct"; }
inline const char* getName(const Variant_& v) noexcept { return "Variant"; }
inline const char* getName(const Graph_& v) noexcept { return "Graph"; }
inline const char* getName(const Optional_& v) noexcept { return "Optional"; }
inline const char* getName(const Container_& v) noexcept { return "Container"; }
inline const char* getName(const Map_& v) noexcept { return "Map"; }
inline const char* getName(const Instance_& v) noexcept { return "Instance"; }
inline const char* getName(const Addressable& v) noexcept { return "Addressable"; }
inline const char* getName(const PolymorphicPair& v) noexcept { return "PolymorphicPair"; }
inline const char* getName(const Polymorphic& v) noexcept { return "Polymorphic"; }
inline const char* getName(const Component& v) noexcept { return "Component"; }
inline const char* getName(const Vector_& v) noexcept { return "Vector"; }
inline const char* getName(const List_& v) noexcept { return "List"; }
inline const char* getName(const Graph2& v) noexcept { return "Graph2"; }
inline const char* getName(const Typescript& v) noexcept { return "Typescript"; }
inline const char* getName(const SyntaxGraph& v) noexcept { return "SyntaxGraph"; }
inline const char* getName(const ModuleInfo& v) noexcept { return "ModuleInfo"; }
inline const char* getName(const ModuleGraph& v) noexcept { return "ModuleGraph"; }

} // namespace Meta

} // namespace Cocos
