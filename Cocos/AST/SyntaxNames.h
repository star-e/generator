/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2022 Xiamen Yaji Software Co., Ltd.

http://www.cocos.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated engine source code (the "Software"), a limited,
worldwide, royalty-free, non-assignable, revocable and non-exclusive license
to use Cocos Creator solely to develop games on your target platforms. You shall
not use Cocos Creator software for developing other software or tools that's
used for developing games. You are not granted to publish, distribute,
sublicense, and/or sell copies of Cocos Creator.

The software or tools in this License Agreement are licensed, not sold.
Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once
#include <Cocos/AST/SyntaxTypes.h>

namespace Cocos {

namespace Meta {

inline const char* getName(const Container& v) noexcept { return "Container"; }
inline const char* getName(const Map& v) noexcept { return "Map"; }
inline const char* getName(const Instance& v) noexcept { return "Instance"; }
inline const char* getName(const Namespace& v) noexcept { return "Namespace"; }
inline const char* getName(const Define& v) noexcept { return "Define"; }
inline const char* getName(const Concept& v) noexcept { return "Concept"; }
inline const char* getName(const Constraints& v) noexcept { return "Constraints"; }
inline const char* getName(const Inherits& v) noexcept { return "Inherits"; }
inline const char* getName(const Alias& v) noexcept { return "Alias"; }
inline const char* getName(const Traits& v) noexcept { return "Traits"; }
inline const char* getName(const Value& v) noexcept { return "Value"; }
inline const char* getName(const EnumValue& v) noexcept { return "EnumValue"; }
inline const char* getName(const Enum& v) noexcept { return "Enum"; }
inline const char* getName(const Tag& v) noexcept { return "Tag"; }
inline const char* getName(const Member& v) noexcept { return "Member"; }
inline const char* getName(const Parameter& v) noexcept { return "Parameter"; }
inline const char* getName(const Method& v) noexcept { return "Method"; }
inline const char* getName(const Constructor& v) noexcept { return "Constructor"; }
inline const char* getName(const Struct& v) noexcept { return "Struct"; }
inline const char* getName(const Variant& v) noexcept { return "Variant"; }
inline const char* getName(const Namespace_& v) noexcept { return "Namespace"; }
inline const char* getName(const Define_& v) noexcept { return "Define"; }
inline const char* getName(const Concept_& v) noexcept { return "Concept"; }
inline const char* getName(const Alias_& v) noexcept { return "Alias"; }
inline const char* getName(const Value_& v) noexcept { return "Value"; }
inline const char* getName(const Enum_& v) noexcept { return "Enum"; }
inline const char* getName(const Tag_& v) noexcept { return "Tag"; }
inline const char* getName(const Struct_& v) noexcept { return "Struct"; }
inline const char* getName(const Variant_& v) noexcept { return "Variant"; }
inline const char* getName(const Graph_& v) noexcept { return "Graph"; }
inline const char* getName(const Container_& v) noexcept { return "Container"; }
inline const char* getName(const Map_& v) noexcept { return "Map"; }
inline const char* getName(const Instance_& v) noexcept { return "Instance"; }
inline const char* getName(const PolymorphicPair& v) noexcept { return "PolymorphicPair"; }
inline const char* getName(const Polymorphic& v) noexcept { return "Polymorphic"; }
inline const char* getName(const Component& v) noexcept { return "Component"; }
inline const char* getName(const VertexMap& v) noexcept { return "VertexMap"; }
inline const char* getName(const Vector_& v) noexcept { return "Vector"; }
inline const char* getName(const List_& v) noexcept { return "List"; }
inline const char* getName(const Set_& v) noexcept { return "Set"; }
inline const char* getName(const MultiSet_& v) noexcept { return "MultiSet"; }
inline const char* getName(const Direct_& v) noexcept { return "Direct"; }
inline const char* getName(const Trie_& v) noexcept { return "Trie"; }
inline const char* getName(const Layer& v) noexcept { return "Layer"; }
inline const char* getName(const Stack& v) noexcept { return "Stack"; }
inline const char* getName(const Named& v) noexcept { return "Named"; }
inline const char* getName(const Addressable& v) noexcept { return "Addressable"; }
inline const char* getName(const MemberRecord& v) noexcept { return "MemberRecord"; }
inline const char* getName(const MemberFormatter& v) noexcept { return "MemberFormatter"; }
inline const char* getName(const Graph& v) noexcept { return "Graph"; }
inline const char* getName(const Typescript& v) noexcept { return "Typescript"; }
inline const char* getName(ImplEnum e) noexcept {
    switch(e) {
    case ImplEnum::None: return "None";
    case ImplEnum::Inline: return "Inline";
    case ImplEnum::Separated: return "Separated";
    case ImplEnum::Delete: return "Delete";
    }
    return "";
}
inline const char* getName(const SyntaxGraph& v) noexcept { return "SyntaxGraph"; }
inline const char* getName(const ModuleInfo& v) noexcept { return "ModuleInfo"; }
inline const char* getName(const ModuleGraph& v) noexcept { return "ModuleGraph"; }

} // namespace Meta

} // namespace Cocos
