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

template <typename T> struct IsComposition { static constexpr bool value = false; };
template <typename T> concept Composition_ = Data_<T> && IsComposition<T>::value;

template <typename T> struct IsTemplate { static constexpr bool value = false; };
template <typename T> concept Template_ = Identifier_<T> && IsTemplate<T>::value;

template <typename T> struct IsInstantiation { static constexpr bool value = false; };
template <typename T> concept Instantiation_ = IsInstantiation<T>::value;

struct Requires_;
struct Composites_;
struct Reuses_;
struct Inherits_;
struct Sums_;
struct References_;
struct Optional;
struct Container;
struct Map;
struct Instance;
struct Namespace;
struct Declare;
struct Alias;
struct Concept;
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
struct Declare_;
struct Concept_;
struct Value_;
struct Enum_;
struct Tag_;
struct Struct_;
struct Variant_;
struct Graph_;
struct Optional_;
struct Container_;
struct Map_;
struct Instance_;
struct Addressable;
struct PolymorphicPair;
struct Polymorphic;
struct Component;
struct Vector_;
struct List_;

using VertexListType = std::variant<Vector_, List_>;

struct Graph2;
struct Typescript;
template <> struct IsIdentifier<Namespace_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Variant_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Namespace> { static constexpr bool value = true; };

template <> struct IsIdentifier<Variant> { static constexpr bool value = true; };

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

template <> struct IsIdentifier<Struct_> { static constexpr bool value = true; };
template <> struct IsData<Struct_> { static constexpr bool value = true; };
template <> struct IsComposition<Struct_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Graph_> { static constexpr bool value = true; };
template <> struct IsData<Graph_> { static constexpr bool value = true; };
template <> struct IsComposition<Graph_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Struct> { static constexpr bool value = true; };
template <> struct IsData<Struct> { static constexpr bool value = true; };
template <> struct IsComposition<Struct> { static constexpr bool value = true; };

template <> struct IsIdentifier<Graph2> { static constexpr bool value = true; };
template <> struct IsData<Graph2> { static constexpr bool value = true; };
template <> struct IsComposition<Graph2> { static constexpr bool value = true; };

template <> struct IsIdentifier<Optional_> { static constexpr bool value = true; };
template <> struct IsTemplate<Optional_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Container_> { static constexpr bool value = true; };
template <> struct IsTemplate<Container_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Map_> { static constexpr bool value = true; };
template <> struct IsTemplate<Map_> { static constexpr bool value = true; };

template <> struct IsIdentifier<Optional> { static constexpr bool value = true; };
template <> struct IsTemplate<Optional> { static constexpr bool value = true; };

template <> struct IsIdentifier<Container> { static constexpr bool value = true; };
template <> struct IsTemplate<Container> { static constexpr bool value = true; };

template <> struct IsIdentifier<Map> { static constexpr bool value = true; };
template <> struct IsTemplate<Map> { static constexpr bool value = true; };

template <> struct IsInstantiation<Instance_> { static constexpr bool value = true; };

template <> struct IsInstantiation<Instance> { static constexpr bool value = true; };

struct SyntaxGraph;
struct ModuleInfo;
struct ModuleGraph;

} // namespace Meta

} // namespace Cocos
