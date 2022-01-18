/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2021 Xiamen Yaji Software Co., Ltd.

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

namespace Cocos {

namespace Meta {

enum GenerationFlags : uint64_t;

template <typename T> struct IsIdentifier { static constexpr bool value = false; };
template <typename T> concept Identifier_ = IsIdentifier<T>::value;

template <typename T> struct IsData { static constexpr bool value = false; };
template <typename T> concept Data_ = Identifier_<T> && IsData<T>::value;

template <typename T> struct IsAlgebra { static constexpr bool value = false; };
template <typename T> concept Algebra_ = Identifier_<T> && IsAlgebra<T>::value;

template <typename T> struct IsComposition { static constexpr bool value = false; };
template <typename T> concept Composition_ = Identifier_<T> && IsComposition<T>::value;

template <typename T> struct IsTemplate { static constexpr bool value = false; };
template <typename T> concept Template_ = Identifier_<T> && IsTemplate<T>::value;

template <typename T> struct IsInstantiation { static constexpr bool value = false; };
template <typename T> concept Instantiation_ = IsInstantiation<T>::value;

struct Container;
struct Map;
struct Instance;
struct Namespace;
struct Define;
struct Concept;
struct Constraints;
struct Inherits;
struct Alias;
struct Traits;
struct Value;
struct EnumValue;
struct Enum;
struct Tag;
struct Member;
struct Constructor;
struct Struct;
struct Variant;
struct Namespace_;
struct Define_;
struct Concept_;
struct Alias_;
struct Value_;
struct Enum_;
struct Tag_;
struct Struct_;
struct Variant_;
struct Graph_;
struct Container_;
struct Map_;
struct Instance_;
struct PolymorphicPair;
struct Polymorphic;
struct Component;
struct VertexMap;

template <typename T> struct IsForward { static constexpr bool value = false; };
template <typename T> concept Forward_ = IsForward<T>::value;

template <typename T> struct IsReversible { static constexpr bool value = false; };
template <typename T> concept Reversible_ = Forward_<T> && IsReversible<T>::value;

template <typename T> struct IsRandomAccess { static constexpr bool value = false; };
template <typename T> concept RandomAccess_ = Reversible_<T> && IsRandomAccess<T>::value;

template <typename T> struct IsSequence { static constexpr bool value = false; };
template <typename T> concept Sequence_ = Forward_<T> && IsSequence<T>::value;

template <typename T> struct IsBackInsertionSequence { static constexpr bool value = false; };
template <typename T> concept BackInsertionSequence_ = Sequence_<T> && IsBackInsertionSequence<T>::value;

template <typename T> struct IsAssociative { static constexpr bool value = false; };
template <typename T> concept Associative_ = Forward_<T> && IsAssociative<T>::value;

template <typename T> struct IsUniqueAssociative { static constexpr bool value = false; };
template <typename T> concept UniqueAssociative_ = Associative_<T> && IsUniqueAssociative<T>::value;

template <typename T> struct IsMultipleAssociative { static constexpr bool value = false; };
template <typename T> concept MultipleAssociative_ = Associative_<T> && IsMultipleAssociative<T>::value;

struct Vector_;
struct List_;
struct Set_;
struct MultiSet_;
template <> struct IsForward<Vector_> { static constexpr bool value = true; };
template <> struct IsSequence<Vector_> { static constexpr bool value = true; };
template <> struct IsBackInsertionSequence<Vector_> { static constexpr bool value = true; };

template <> struct IsForward<List_> { static constexpr bool value = true; };
template <> struct IsSequence<List_> { static constexpr bool value = true; };
template <> struct IsBackInsertionSequence<List_> { static constexpr bool value = true; };

template <> struct IsForward<Set_> { static constexpr bool value = true; };
template <> struct IsAssociative<Set_> { static constexpr bool value = true; };
template <> struct IsUniqueAssociative<Set_> { static constexpr bool value = true; };

template <> struct IsForward<MultiSet_> { static constexpr bool value = true; };
template <> struct IsAssociative<MultiSet_> { static constexpr bool value = true; };
template <> struct IsMultipleAssociative<MultiSet_> { static constexpr bool value = true; };

using VertexListType = std::variant<Vector_, List_>;
using EdgeListType = std::variant<List_, Set_, MultiSet_>;
using OutEdgeListType = std::variant<Vector_, List_, Set_, MultiSet_>;

struct Direct_;
struct Trie_;

using PathIndexType = std::variant<Direct_, Map_>;

struct Layer;
struct Stack;
struct Named;
struct Addressable;
struct Graph;
struct Typescript;
template <> struct IsIdentifier<Define_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Namespace_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Concept_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Alias_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Define> { static constexpr bool value = true; };

template <> struct IsIdentifier<Namespace> { static constexpr bool value = true; };

template <> struct IsIdentifier<Concept> { static constexpr bool value = true; };

template <> struct IsIdentifier<Alias> { static constexpr bool value = true; };

template <> struct IsIdentifier<Value_> { static constexpr bool value = true; };
template <> struct IsData<Value_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Enum_> { static constexpr bool value = true; };
template <> struct IsData<Enum_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Tag_> { static constexpr bool value = true; };
template <> struct IsData<Tag_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Value> { static constexpr bool value = true; };
template <> struct IsData<Value> { static constexpr bool value = true; };

template <> struct IsIdentifier<Enum> { static constexpr bool value = true; };
template <> struct IsData<Enum> { static constexpr bool value = true; };

template <> struct IsIdentifier<Tag> { static constexpr bool value = true; };
template <> struct IsData<Tag> { static constexpr bool value = true; };

template <> struct IsIdentifier<Variant_> { static constexpr bool value = true; };
template <> struct IsAlgebra<Variant_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Variant> { static constexpr bool value = true; };
template <> struct IsAlgebra<Variant> { static constexpr bool value = true; };

template <> struct IsIdentifier<Struct_> { static constexpr bool value = true; };
template <> struct IsComposition<Struct_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Graph_> { static constexpr bool value = true; };
template <> struct IsComposition<Graph_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Struct> { static constexpr bool value = true; };
template <> struct IsComposition<Struct> { static constexpr bool value = true; };

template <> struct IsIdentifier<Graph> { static constexpr bool value = true; };
template <> struct IsComposition<Graph> { static constexpr bool value = true; };

template <> struct IsIdentifier<Container_> { static constexpr bool value = true; };
template <> struct IsTemplate<Container_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Map_> { static constexpr bool value = true; };
template <> struct IsTemplate<Map_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Container> { static constexpr bool value = true; };
template <> struct IsTemplate<Container> { static constexpr bool value = true; };

template <> struct IsIdentifier<Map> { static constexpr bool value = true; };
template <> struct IsTemplate<Map> { static constexpr bool value = true; };

template <> struct IsInstantiation<Instance_> { static constexpr bool value = true; };

template <> struct IsInstantiation<Instance> { static constexpr bool value = true; };

enum class ImplEnum : uint32_t;

struct SyntaxGraph;

enum Features : uint32_t;

struct ModuleInfo;
struct ModuleGraph;

} // namespace Meta

} // namespace Cocos
