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
#pragma warning(disable : 4390)

// clang-format off

// Helpers
#define COMMA_STRINGIZE_ELEM(r, _, i, C) BOOST_PP_COMMA_IF(i) BOOST_PP_STRINGIZE(C)

// Builder
#define MODULE(NAME, ...) \
    if (auto m = builder.openModule(BOOST_PP_STRINGIZE(NAME), \
        ModuleInfo{ .mFeatures = features, __VA_ARGS__ }); true)

#define ADD_FEATURES(...) \
    get(ModuleGraph::modules, builder.mModuleGraph, m.mVertexDescriptor).mFeatures |= __VA_ARGS__

#define NAMESPACE(NAME) \
    if (auto ns = builder.openNamespace(BOOST_PP_STRINGIZE(NAME)); true)

#define NAMESPACE_BEG(NAME) { \
    auto ns = builder.openNamespace(BOOST_PP_STRINGIZE(NAME))

#define NAMESPACE_END(...) }

#define VALUE(NAME) \
    builder.addValue(BOOST_PP_STRINGIZE(NAME))

#define ALIAS(NAME, TYPE) \
    builder.addAlias(BOOST_PP_STRINGIZE(NAME), BOOST_PP_STRINGIZE(TYPE))

#define ENUM_MEMBER(r, _, i, MEMBER)       \
    builder.addEnumElement(vertID, BOOST_PP_STRINGIZE(MEMBER), "");

#define ENUM(NAME, ...) \
    if (auto vertID = builder.addEnum(BOOST_PP_STRINGIZE(NAME), Traits{ __VA_ARGS__ }); true)

#define ENUM_CLASS(NAME, ...) \
    if (auto vertID = builder.addEnum(BOOST_PP_STRINGIZE(NAME), Traits{ .mClass = true, __VA_ARGS__ }); true)

#define ENUMS(...) \
    BOOST_PP_SEQ_FOR_EACH_I(ENUM_MEMBER, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)))

#define UNDERLYING_TYPE(NAME) \
    builder.setEnumUnderlyingType(vertID, BOOST_PP_STRINGIZE(NAME))

#define FLAG_MEMBER(r, _, i, MEMBER) \
builder.addEnumElement(vertID, \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2, 0, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(2, 1, MEMBER)));

#define FLAG_MEMBER3(r, _, i, MEMBER) \
builder.addEnumElement(vertID, \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 0, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 1, MEMBER)));

#define FLAG(NAME, ...) \
    if (auto vertID = builder.addFlag(BOOST_PP_STRINGIZE(NAME), Traits{ __VA_ARGS__ }); true)

#define FLAG_CLASS(NAME, ...) \
    if (auto vertID = builder.addFlag(BOOST_PP_STRINGIZE(NAME), Traits{ .mClass = true, __VA_ARGS__ }); true)

#define FLAGS(SEQ) \
    BOOST_PP_SEQ_FOR_EACH_I(FLAG_MEMBER, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQ))

#define IMPORT_ENUM(NAME) \
    if (auto vertID = builder.addEnum(BOOST_PP_STRINGIZE(NAME), \
        Traits{ .mImport = true }); true)

#define IMPORT_FLAGS(NAME) \
    if (auto vertID = builder.addFlag(BOOST_PP_STRINGIZE(NAME), \
        Traits{ .mImport = true }); true)

#define IMPORT_CLASS(NAME, ...) \
    if (auto s = builder.addStruct(BOOST_PP_STRINGIZE(NAME),\
        Traits{ .mClass = true, .mImport = true, __VA_ARGS__ }); true)

#define IMPORT_PMR_CLASS(NAME, ...) \
    if (auto s = builder.addStruct(BOOST_PP_STRINGIZE(NAME),\
        Traits{ .mClass = true, .mImport = true, .mPmr = true, __VA_ARGS__ }); true)

#define IMPORT_STRUCT(NAME, ...) \
    if (auto s = builder.addStruct(BOOST_PP_STRINGIZE(NAME),\
        Traits{ .mClass = false,.mImport = true,  __VA_ARGS__ }); true)

#define IMPORT_PMR_STRUCT(NAME, ...) \
    if (auto s = builder.addStruct(BOOST_PP_STRINGIZE(NAME),\
        Traits{ .mClass = false, .mImport = true, .mPmr = true, __VA_ARGS__ }); true)

#define STRUCT(NAME, ...) \
    for (auto&& [vertID, mb, path] = builder.addStruct(BOOST_PP_STRINGIZE(NAME), Traits{ __VA_ARGS__ }); \
        vertID != SyntaxGraph::null_vertex(); \
        builder.syntax().propagate(vertID), \
        vertID = SyntaxGraph::null_vertex())

#define CLASS(NAME, ...) STRUCT(NAME, .mClass = true, __VA_ARGS__)

#define MEMBER_FLAGS(MEMBER, ...) \
    builder.setMemberFlags(vertID, BOOST_PP_STRINGIZE(MEMBER), __VA_ARGS__)

#define CNTR_MEMBER(r, _, i, MEMBER) BOOST_PP_STRINGIZE(MEMBER),

#define CNTR(...) \
builder.addConstructor(vertID, {\
BOOST_PP_SEQ_FOR_EACH_I(CNTR_MEMBER, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__))) }, true)

#define CNTR_EMPTY() \
builder.addConstructor(vertID, {}, true)

#define CNTR_NO_DEFAULT(...) \
builder.addConstructor(vertID, {\
BOOST_PP_SEQ_FOR_EACH_I(CNTR_MEMBER, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__))) }, false)

#define CONTAINER(NAME, ...) \
    builder.addContainer(BOOST_PP_STRINGIZE(NAME), Traits{ __VA_ARGS__ })

#define PMR_CONTAINER(NAME, ...) \
    builder.addContainer(BOOST_PP_STRINGIZE(NAME), Traits{ .mPmr = true, __VA_ARGS__ })

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

// Concept
#define REQUIRES_ELEM(r, _, i, SUPERTYPE) builder.addConstraints(vertID, BOOST_PP_STRINGIZE(SUPERTYPE));
#define REQUIRES(...) BOOST_PP_SEQ_FOR_EACH_I(REQUIRES_ELEM, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)))

// Interface
#define INTERFACE(NAME, ...) \
    for (auto&& [vertID, mb, path] = builder.addStruct(BOOST_PP_STRINGIZE(NAME), \
        Traits{ .mClass = true, .mInterface = true, __VA_ARGS__ }); \
        vertID != SyntaxGraph::null_vertex(); \
        builder.syntax().propagate(vertID), \
        vertID = SyntaxGraph::null_vertex())

// Inheritance
#define INHERITS_ELEM(r, VIRTUAL, i, C) builder.addInherits(vertID, BOOST_PP_STRINGIZE(C), VIRTUAL);
#define INHERITS(...) \
    BOOST_PP_SEQ_FOR_EACH_I(INHERITS_ELEM, false, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)))

#define VIRTUAL_INHERITS(...) \
    BOOST_PP_SEQ_FOR_EACH_I(INHERITS_ELEM, true, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)))

// Implements
#define IMPLEMENTS_ELEM(r, VIRTUAL, i, C) builder.addInherits(vertID, BOOST_PP_STRINGIZE(C), false, true);
#define IMPLEMENTS(...) \
    BOOST_PP_SEQ_FOR_EACH_I(IMPLEMENTS_ELEM, false, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)))

#define VIRTUAL_IMPLEMENTS(...) \
    BOOST_PP_SEQ_FOR_EACH_I(IMPLEMENTS_ELEM, true, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__)))

// Struct
#define STRUCT_MEMBER(r, COND, i, MEMBER) \
builder.addMember(vertID, COND,\
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 0, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 1, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 2, MEMBER)), \
    {});

#define MEMBERS(SEQ) \
    BOOST_PP_SEQ_FOR_EACH_I(STRUCT_MEMBER, true, BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQ))

#define PUBLIC MEMBERS
#define PRIVATE(SEQ) \
    BOOST_PP_SEQ_FOR_EACH_I(STRUCT_MEMBER, false, BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQ))

#define MEMBER_FUNCTIONS(STR) \
    builder.addMemberFunctions(vertID, STR)

#define PUBLIC_METHODS(STR) \
    builder.addMethods(vertID, STR)

#define TS_FUNCTIONS(STR) \
    builder.addTypescriptFunctions(vertID, STR)

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
    for (auto&& [vertID, mb, path] = builder.addGraph(BOOST_PP_STRINGIZE(NAME),\
        BOOST_PP_STRINGIZE(VERTEX), BOOST_PP_STRINGIZE(EDGE), Traits{ __VA_ARGS__ }); \
        vertID != SyntaxGraph::null_vertex(); \
        builder.syntax().propagate(vertID), \
        vertID = SyntaxGraph::null_vertex()) \
        if (auto& graph = get_by_tag<Graph_>(vertID, builder.mSyntaxGraph); true) \

#define PMR_GRAPH(NAME, VERTEX, EDGE, ...) GRAPH(NAME, VERTEX, EDGE, .mPmr = true, __VA_ARGS__)

#define OBJECT_DESCRIPTOR() \
    graph.mVertexListType = List_ {}

#define NAMED_GRAPH(...)                               \
    builder.addNamedConcept(vertID, true, \
        BOOST_PP_SEQ_FOR_EACH_I(COMMA_STRINGIZE_ELEM, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__))))

#define GRAPH_COMPONENT_ELEM(r, _, i, MEMBER) \
builder.addGraphComponent(vertID, \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 0, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 1, MEMBER)), \
    BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(3, 2, MEMBER)) \
);

#define COMPONENT_GRAPH(SEQ) \
BOOST_PP_SEQ_FOR_EACH_I(GRAPH_COMPONENT_ELEM, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(SEQ))

#define POLYMORPHIC_GRAPH_ELEM(r, _, i, MEMBER) \
builder.addGraphPolymorphic(vertID, \
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

#define ADDRESSABLE_GRAPH(MEMBER) \
    graph.mReferenceGraph = true; \
    graph.mAddressable = true; \
    graph.mAddressableConcept.mMemberName = BOOST_PP_STRINGIZE(MEMBER)

#define COMPONENT_BIMAP(MAPTYPE, MEMBER, ...) \
    builder.addVertexBimap(vertID,\
        BOOST_PP_STRINGIZE(MAPTYPE), BOOST_PP_STRINGIZE(MEMBER), \
        BOOST_PP_SEQ_FOR_EACH_I(COMMA_STRINGIZE_ELEM, _, BOOST_PP_TUPLE_TO_SEQ((__VA_ARGS__))))

// Typescript
#define PROJECT_TS(TYPE, JS) \
	builder.projectTypescript(BOOST_PP_STRINGIZE(TYPE), BOOST_PP_STRINGIZE(JS))

#define TS_PROJECT PROJECT_TS

#define TS_INIT(MEMBER, VALUE) \
    builder.setTypescriptInitValue(vertID, \
        BOOST_PP_STRINGIZE(MEMBER), BOOST_PP_STRINGIZE(VALUE))

// clang-format on
