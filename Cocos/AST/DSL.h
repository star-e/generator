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
#pragma warning(disable : 4390)

#define MODULE(NAME, ...) \
    if (auto m = builder.openModule(BOOST_PP_STRINGIZE(NAME), ModuleInfo{ __VA_ARGS__ }); true)

#define NAMESPACE(NAME) \
    if (auto ns = builder.openNamespace(BOOST_PP_STRINGIZE(NAME)); true)

#define VALUE(NAME) \
    builder.addValue(BOOST_PP_STRINGIZE(NAME))

#define ENUM_MEMBER(r, _, i, MEMBER)       \
    builder.addEnumElement(vertID, BOOST_PP_STRINGIZE(MEMBER));

#define ENUM(NAME, TUPLE) \
    if (auto vertID = builder.addEnum(BOOST_PP_STRINGIZE(NAME)); true) { \
        BOOST_PP_SEQ_FOR_EACH_I(ENUM_MEMBER, _, BOOST_PP_TUPLE_TO_SEQ(TUPLE)) \
    }

#define FLAGS_ELEM(r, _, i, MEMBER) \
builder.addEnumElement(vertID, \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2, 0, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2, 1, MEMBER)));

#define FLAGS(NAME, SEQ) \
    if (auto vertID = builder.addFlags(BOOST_PP_STRINGIZE(NAME)); true) { \
        BOOST_PP_SEQ_FOR_EACH_I(FLAGS_ELEM, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQ)) \
    }

#define IMPORT_ENUM(NAME) \
    if (auto vertID = builder.addEnum(BOOST_PP_STRINGIZE(NAME), \
        Traits{ .mImport = true }); true)

#define IMPORT_FLAGS(NAME) \
    if (auto vertID = builder.addFlags(BOOST_PP_STRINGIZE(NAME), \
        Traits{ .mImport = true }); true)

#define IMPORT_CLASS(NAME) \
    if (auto s = builder.addStruct(BOOST_PP_STRINGIZE(NAME),\
        Traits{ .mImport = true, .mClass = true }); true)

#define IMPORT_PMR_CLASS(NAME) \
    if (auto s = builder.addStruct(BOOST_PP_STRINGIZE(NAME),\
        Traits{ .mImport = true, .mPmr = true, .mClass = true }); true)

#define IMPORT_STRUCT(NAME) \
    if (auto s = builder.addStruct(BOOST_PP_STRINGIZE(NAME),\
        Traits{ .mImport = true, .mClass = false }); true)

#define IMPORT_PMR_STRUCT(NAME) \
    if (auto s = builder.addStruct(BOOST_PP_STRINGIZE(NAME),\
        Traits{ .mImport = true, .mPmr = true, .mClass = false }); true)

#define STRUCT(NAME, ...) \
    if (auto s = builder.addStruct(BOOST_PP_STRINGIZE(NAME), __VA_ARGS__); true)

#define CNTR_MEMBER(r, _, i, MEMBER) BOOST_PP_STRINGIZE(MEMBER),

#define CNTR(...) \
builder.addConstructor(s.mVertexDescriptor, {\
BOOST_PP_SEQ_FOR_EACH_I(CNTR_MEMBER, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__))) })

#define CONTAINER(NAME) \
    builder.addContainer(BOOST_PP_STRINGIZE(NAME), Traits{})

#define PMR_CONTAINER(NAME) \
    builder.addContainer(BOOST_PP_STRINGIZE(NAME), Traits{ .mPmr = true })

#define MAP(NAME) \
    builder.addMap(BOOST_PP_STRINGIZE(NAME), Traits{})

#define PMR_MAP(NAME) \
    builder.addMap(BOOST_PP_STRINGIZE(NAME), Traits{ .mPmr = true })

// Tag
#define TAG(NAME, ...) \
    builder.addTag(BOOST_PP_STRINGIZE(NAME), __VA_ARGS__)

#define STRINGIZE_ELEM(r, _, C) BOOST_PP_STRINGIZE(C),
#define TAG_ELEM(r, Cs, i, NAME) builder.addTag(BOOST_PP_STRINGIZE(NAME), \
    false, { BOOST_PP_REMOVE_PARENS(Cs) });
#define TAGS(Cs, ...) BOOST_PP_SEQ_FOR_EACH_I(TAG_ELEM, (BOOST_PP_SEQ_FOR_EACH(STRINGIZE_ELEM, _, BOOST_PP_TUPLE_TO_SEQ(Cs))), BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)))

// Struct
#define STRUCT_MEMBER(r, COND, i, MEMBER) \
builder.addMember(s.mVertexDescriptor, COND,\
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 0, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 1, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 2, MEMBER)), \
    {});

#define MEMBERS(SEQ) \
    BOOST_PP_SEQ_FOR_EACH_I(STRUCT_MEMBER, true, BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQ))

#define PUBLIC MEMBERS
#define PRIVATE(SEQ) \
    BOOST_PP_SEQ_FOR_EACH_I(STRUCT_MEMBER, false, BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQ))

#define TS_FUNCTIONS(STR) \
    builder.addTypescriptFunctions(s.mVertexDescriptor, STR)

// Variant
#define VARIANT_ELEM(r, V, i, MEMBER) \
    builder.addVariantElement(variantID, BOOST_PP_STRINGIZE(MEMBER));

#define VARIANT_MEMBERS(NAME, TUPLE) \
    BOOST_PP_SEQ_FOR_EACH_I(VARIANT_ELEM, NAME, BOOST_PP_TUPLE_TO_SEQ(TUPLE))

#define VARIANT(NAME, TUPLE, ...) \
    if (auto variantID = builder.addVariant(BOOST_PP_STRINGIZE(NAME), __VA_ARGS__); true) { \
        BOOST_PP_SEQ_FOR_EACH_I(VARIANT_ELEM, NAME, BOOST_PP_TUPLE_TO_SEQ(TUPLE)) \
    }

// Graph
#define GRAPH(NAME, VERTEX, EDGE, ...) \
    if (auto s = builder.addGraph(BOOST_PP_STRINGIZE(NAME),\
        BOOST_PP_STRINGIZE(VERTEX), BOOST_PP_STRINGIZE(EDGE), Traits{ __VA_ARGS__ }); true) \
        if (auto& graph = get_by_tag<Graph_>(s.mVertexDescriptor, builder.mSyntaxGraph); true) \

#define OBJECT_DESCRIPTOR() \
    graph.mVertexListType = List_ {}

#define NAMED_GRAPH() \
    graph.mNamed = true

#define GRAPH_COMPONENT_ELEM(r, _, i, MEMBER) \
builder.addGraphComponent(s.mVertexDescriptor, \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 0, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 1, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 2, MEMBER)) \
);

#define COMPONENT_GRAPH(SEQ) \
BOOST_PP_SEQ_FOR_EACH_I(GRAPH_COMPONENT_ELEM, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQ))

#define POLYMORPHIC_GRAPH_ELEM(r, _, i, MEMBER) \
builder.addGraphPolymorphic(s.mVertexDescriptor, \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 0, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 1, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 2, MEMBER)));

#define POLYMORPHIC_GRAPH(SEQ) \
BOOST_PP_SEQ_FOR_EACH_I(POLYMORPHIC_GRAPH_ELEM, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQ))

#define REFERENCE_GRAPH() \
    graph.mReferenceGraph = true; \
    graph.mAliasGraph = false

#define ALIAS_REFERENCE_GRAPH() \
    graph.mReferenceGraph = true; \
    graph.mAliasGraph = true

#define ADDRESSABLE_GRAPH() \
    graph.mAddressable.emplace()

// Typescript
#define PROJECT_TS(TYPE, JS) \
	builder.projectTypescript(BOOST_PP_STRINGIZE(TYPE), BOOST_PP_STRINGIZE(JS))
