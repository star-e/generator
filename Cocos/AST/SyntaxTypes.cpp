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

#include "SyntaxTypes.h"

namespace Cocos {

namespace Meta {

Instance::Instance(const allocator_type& alloc) noexcept
    : mParameters(alloc) {}

Instance::Instance(Instance&& rhs, const allocator_type& alloc)
    : mParameters(std::move(rhs.mParameters), alloc) {}

Instance::Instance(Instance const& rhs, const allocator_type& alloc)
    : mParameters(rhs.mParameters, alloc) {}

Instance::~Instance() noexcept = default;

Alias::Alias(const allocator_type& alloc) noexcept
    : mName(alloc) {}

Alias::Alias(Alias&& rhs, const allocator_type& alloc)
    : mName(std::move(rhs.mName), alloc) {}

Alias::Alias(Alias const& rhs, const allocator_type& alloc)
    : mName(rhs.mName, alloc) {}

Alias::~Alias() noexcept = default;

EnumValue::EnumValue(const allocator_type& alloc) noexcept
    : mName(alloc)
    , mValue(alloc)
    , mReflectionName(alloc) {}

EnumValue::EnumValue(EnumValue&& rhs, const allocator_type& alloc)
    : mName(std::move(rhs.mName), alloc)
    , mValue(std::move(rhs.mValue), alloc)
    , mReflectionName(std::move(rhs.mReflectionName), alloc) {}

EnumValue::EnumValue(EnumValue const& rhs, const allocator_type& alloc)
    : mName(rhs.mName, alloc)
    , mValue(rhs.mValue, alloc)
    , mReflectionName(rhs.mReflectionName, alloc) {}

EnumValue::~EnumValue() noexcept = default;

Enum::Enum(const allocator_type& alloc) noexcept
    : mUnderlyingType(alloc)
    , mValues(alloc) {}

Enum::Enum(Enum&& rhs, const allocator_type& alloc)
    : mIsFlags(std::move(rhs.mIsFlags))
    , mEnumOperator(std::move(rhs.mEnumOperator))
    , mUnderlyingType(std::move(rhs.mUnderlyingType), alloc)
    , mValues(std::move(rhs.mValues), alloc) {}

Enum::Enum(Enum const& rhs, const allocator_type& alloc)
    : mIsFlags(rhs.mIsFlags)
    , mEnumOperator(rhs.mEnumOperator)
    , mUnderlyingType(rhs.mUnderlyingType, alloc)
    , mValues(rhs.mValues, alloc) {}

Enum::~Enum() noexcept = default;

Member::Member(const allocator_type& alloc) noexcept
    : mTypePath(alloc)
    , mMemberName(alloc)
    , mDefaultValue(alloc)
    , mTypescriptType(alloc)
    , mTypescriptDefaultValue(alloc) {}

Member::Member(Member&& rhs, const allocator_type& alloc)
    : mTypePath(std::move(rhs.mTypePath), alloc)
    , mMemberName(std::move(rhs.mMemberName), alloc)
    , mDefaultValue(std::move(rhs.mDefaultValue), alloc)
    , mConst(std::move(rhs.mConst))
    , mPointer(std::move(rhs.mPointer))
    , mPublic(std::move(rhs.mPublic))
    , mFlags(std::move(rhs.mFlags))
    , mTypescriptType(std::move(rhs.mTypescriptType), alloc)
    , mTypescriptDefaultValue(std::move(rhs.mTypescriptDefaultValue), alloc)
    , mTypescriptArray(std::move(rhs.mTypescriptArray)) {}

Member::Member(Member const& rhs, const allocator_type& alloc)
    : mTypePath(rhs.mTypePath, alloc)
    , mMemberName(rhs.mMemberName, alloc)
    , mDefaultValue(rhs.mDefaultValue, alloc)
    , mConst(rhs.mConst)
    , mPointer(rhs.mPointer)
    , mPublic(rhs.mPublic)
    , mFlags(rhs.mFlags)
    , mTypescriptType(rhs.mTypescriptType, alloc)
    , mTypescriptDefaultValue(rhs.mTypescriptDefaultValue, alloc)
    , mTypescriptArray(rhs.mTypescriptArray) {}

Member::~Member() noexcept = default;

Constructor::Constructor(const allocator_type& alloc) noexcept
    : mIndices(alloc) {}

Constructor::Constructor(Constructor&& rhs, const allocator_type& alloc)
    : mIndices(std::move(rhs.mIndices), alloc) {}

Constructor::Constructor(Constructor const& rhs, const allocator_type& alloc)
    : mIndices(rhs.mIndices, alloc) {}

Constructor::~Constructor() noexcept = default;

Struct::Struct(const allocator_type& alloc) noexcept
    : mMembers(alloc)
    , mConstructors(alloc)
    , mTypescriptMembers(alloc)
    , mTypescriptFunctions(alloc) {}

Struct::Struct(Struct&& rhs, const allocator_type& alloc)
    : mMembers(std::move(rhs.mMembers), alloc)
    , mConstructors(std::move(rhs.mConstructors), alloc)
    , mTypescriptMembers(std::move(rhs.mTypescriptMembers), alloc)
    , mTypescriptFunctions(std::move(rhs.mTypescriptFunctions), alloc) {}

Struct::Struct(Struct const& rhs, const allocator_type& alloc)
    : mMembers(rhs.mMembers, alloc)
    , mConstructors(rhs.mConstructors, alloc)
    , mTypescriptMembers(rhs.mTypescriptMembers, alloc)
    , mTypescriptFunctions(rhs.mTypescriptFunctions, alloc) {}

Struct::~Struct() noexcept = default;

Variant::Variant(const allocator_type& alloc) noexcept
    : mVariants(alloc) {}

Variant::Variant(Variant&& rhs, const allocator_type& alloc)
    : mVariants(std::move(rhs.mVariants), alloc)
    , mUseIndex(std::move(rhs.mUseIndex)) {}

Variant::Variant(Variant const& rhs, const allocator_type& alloc)
    : mVariants(rhs.mVariants, alloc)
    , mUseIndex(rhs.mUseIndex) {}

Variant::~Variant() noexcept = default;

PolymorphicPair::PolymorphicPair(const allocator_type& alloc) noexcept
    : mTag(alloc)
    , mValue(alloc)
    , mMemberName(alloc) {}

PolymorphicPair::PolymorphicPair(PolymorphicPair&& rhs, const allocator_type& alloc)
    : mTag(std::move(rhs.mTag), alloc)
    , mValue(std::move(rhs.mValue), alloc)
    , mMemberName(std::move(rhs.mMemberName), alloc) {}

PolymorphicPair::PolymorphicPair(PolymorphicPair const& rhs, const allocator_type& alloc)
    : mTag(rhs.mTag, alloc)
    , mValue(rhs.mValue, alloc)
    , mMemberName(rhs.mMemberName, alloc) {}

PolymorphicPair::~PolymorphicPair() noexcept = default;

Polymorphic::Polymorphic(const allocator_type& alloc) noexcept
    : mConcepts(alloc) {}

Polymorphic::Polymorphic(Polymorphic&& rhs, const allocator_type& alloc)
    : mConcepts(std::move(rhs.mConcepts), alloc) {}

Polymorphic::Polymorphic(Polymorphic const& rhs, const allocator_type& alloc)
    : mConcepts(rhs.mConcepts, alloc) {}

Polymorphic::~Polymorphic() noexcept = default;

Component::Component(const allocator_type& alloc) noexcept
    : mName(alloc)
    , mValuePath(alloc)
    , mMemberName(alloc) {}

Component::Component(Component&& rhs, const allocator_type& alloc)
    : mName(std::move(rhs.mName), alloc)
    , mValuePath(std::move(rhs.mValuePath), alloc)
    , mMemberName(std::move(rhs.mMemberName), alloc) {}

Component::Component(Component const& rhs, const allocator_type& alloc)
    : mName(rhs.mName, alloc)
    , mValuePath(rhs.mValuePath, alloc)
    , mMemberName(rhs.mMemberName, alloc) {}

Component::~Component() noexcept = default;

Graph2::Graph2(const allocator_type& alloc) noexcept
    : mMembers(alloc)
    , mConstructors(alloc)
    , mTypescriptMembers(alloc)
    , mTypescriptFunctions(alloc)
    , mVertexProperty(alloc)
    , mEdgeProperty(alloc)
    , mComponents(alloc)
    , mPolymorphic(alloc)
    , mVertexDescriptor(alloc)
    , mVertexSizeType(alloc)
    , mDifferenceType(alloc)
    , mEdgeSizeType(alloc)
    , mEdgeDifferenceType(alloc)
    , mDegreeSizeType(alloc) {}

Graph2::Graph2(Graph2&& rhs, const allocator_type& alloc)
    : mMembers(std::move(rhs.mMembers), alloc)
    , mConstructors(std::move(rhs.mConstructors), alloc)
    , mTypescriptMembers(std::move(rhs.mTypescriptMembers), alloc)
    , mTypescriptFunctions(std::move(rhs.mTypescriptFunctions), alloc)
    , mVertexProperty(std::move(rhs.mVertexProperty), alloc)
    , mEdgeProperty(std::move(rhs.mEdgeProperty), alloc)
    , mComponents(std::move(rhs.mComponents), alloc)
    , mNamed(std::move(rhs.mNamed))
    , mReferenceGraph(std::move(rhs.mReferenceGraph))
    , mAliasGraph(std::move(rhs.mAliasGraph))
    , mMutableReference(std::move(rhs.mMutableReference))
    , mAddressable(std::move(rhs.mAddressable))
    , mPolymorphic(std::move(rhs.mPolymorphic), alloc)
    , mVertexListType(std::move(rhs.mVertexListType))
    , mVertexDescriptor(std::move(rhs.mVertexDescriptor), alloc)
    , mVertexSizeType(std::move(rhs.mVertexSizeType), alloc)
    , mDifferenceType(std::move(rhs.mDifferenceType), alloc)
    , mEdgeSizeType(std::move(rhs.mEdgeSizeType), alloc)
    , mEdgeDifferenceType(std::move(rhs.mEdgeDifferenceType), alloc)
    , mDegreeSizeType(std::move(rhs.mDegreeSizeType), alloc) {}

Graph2::Graph2(Graph2 const& rhs, const allocator_type& alloc)
    : mMembers(rhs.mMembers, alloc)
    , mConstructors(rhs.mConstructors, alloc)
    , mTypescriptMembers(rhs.mTypescriptMembers, alloc)
    , mTypescriptFunctions(rhs.mTypescriptFunctions, alloc)
    , mVertexProperty(rhs.mVertexProperty, alloc)
    , mEdgeProperty(rhs.mEdgeProperty, alloc)
    , mComponents(rhs.mComponents, alloc)
    , mNamed(rhs.mNamed)
    , mReferenceGraph(rhs.mReferenceGraph)
    , mAliasGraph(rhs.mAliasGraph)
    , mMutableReference(rhs.mMutableReference)
    , mAddressable(rhs.mAddressable)
    , mPolymorphic(rhs.mPolymorphic, alloc)
    , mVertexListType(rhs.mVertexListType)
    , mVertexDescriptor(rhs.mVertexDescriptor, alloc)
    , mVertexSizeType(rhs.mVertexSizeType, alloc)
    , mDifferenceType(rhs.mDifferenceType, alloc)
    , mEdgeSizeType(rhs.mEdgeSizeType, alloc)
    , mEdgeDifferenceType(rhs.mEdgeDifferenceType, alloc)
    , mDegreeSizeType(rhs.mDegreeSizeType, alloc) {}

Graph2::~Graph2() noexcept = default;

Typescript::Typescript(const allocator_type& alloc) noexcept
    : mName(alloc) {}

Typescript::Typescript(Typescript&& rhs, const allocator_type& alloc)
    : mName(std::move(rhs.mName), alloc)
    , mArray(std::move(rhs.mArray)) {}

Typescript::Typescript(Typescript const& rhs, const allocator_type& alloc)
    : mName(rhs.mName, alloc)
    , mArray(rhs.mArray) {}

Typescript::~Typescript() noexcept = default;

SyntaxGraph::allocator_type SyntaxGraph::get_allocator() const noexcept {
    return allocator_type(mVertices.get_allocator().resource());
}

SyntaxGraph::SyntaxGraph(const allocator_type& alloc)
    : mObjects(alloc)
    , mVertices(alloc)
    , mNames(alloc)
    , mTraits(alloc)
    , mModulePaths(alloc)
    , mTypescripts(alloc)
    , mEnums(alloc)
    , mTags(alloc)
    , mStructs(alloc)
    , mGraphs(alloc)
    , mVariants(alloc)
    , mInstances(alloc)
    , mPathIndex(alloc) {}

SyntaxGraph::SyntaxGraph(SyntaxGraph&& rhs, const allocator_type& alloc)
    : mObjects(std::move(rhs.mObjects), alloc)
    , mVertices(std::move(rhs.mVertices), alloc)
    , mNames(std::move(rhs.mNames), alloc)
    , mTraits(std::move(rhs.mTraits), alloc)
    , mModulePaths(std::move(rhs.mModulePaths), alloc)
    , mTypescripts(std::move(rhs.mTypescripts), alloc)
    , mEnums(std::move(rhs.mEnums), alloc)
    , mTags(std::move(rhs.mTags), alloc)
    , mStructs(std::move(rhs.mStructs), alloc)
    , mGraphs(std::move(rhs.mGraphs), alloc)
    , mVariants(std::move(rhs.mVariants), alloc)
    , mInstances(std::move(rhs.mInstances), alloc)
    , mPathIndex(std::move(rhs.mPathIndex), alloc) {}

SyntaxGraph::SyntaxGraph(SyntaxGraph const& rhs, const allocator_type& alloc)
    : mObjects(rhs.mObjects, alloc)
    , mVertices(rhs.mVertices, alloc)
    , mNames(rhs.mNames, alloc)
    , mTraits(rhs.mTraits, alloc)
    , mModulePaths(rhs.mModulePaths, alloc)
    , mTypescripts(rhs.mTypescripts, alloc)
    , mEnums(rhs.mEnums, alloc)
    , mTags(rhs.mTags, alloc)
    , mStructs(rhs.mStructs, alloc)
    , mGraphs(rhs.mGraphs, alloc)
    , mVariants(rhs.mVariants, alloc)
    , mInstances(rhs.mInstances, alloc)
    , mPathIndex(rhs.mPathIndex, alloc) {}

SyntaxGraph::SyntaxGraph(SyntaxGraph&& rhs) = default;
SyntaxGraph::~SyntaxGraph() noexcept = default;
SyntaxGraph& SyntaxGraph::operator=(SyntaxGraph&& rhs) = default;
SyntaxGraph& SyntaxGraph::operator=(SyntaxGraph const& rhs) = default;

SyntaxGraph::object_type::allocator_type SyntaxGraph::object_type::get_allocator() const noexcept {
    return allocator_type{ mChildren.get_allocator().resource() };
}

SyntaxGraph::object_type::object_type(const allocator_type& alloc)
    : mChildren(alloc)
    , mParents(alloc) {}

SyntaxGraph::object_type::object_type(object_type&& rhs, const allocator_type& alloc)
    : mChildren(std::move(rhs.mChildren), alloc)
    , mParents(std::move(rhs.mParents), alloc) {}

SyntaxGraph::object_type::object_type(const object_type& rhs, const allocator_type& alloc)
    : mChildren(rhs.mChildren, alloc)
    , mParents(rhs.mParents, alloc) {}

SyntaxGraph::object_type::object_type(object_type&&) noexcept = default;
SyntaxGraph::object_type& SyntaxGraph::object_type::operator=(object_type&&) = default;
SyntaxGraph::object_type& SyntaxGraph::object_type::operator=(object_type const&) = default;
SyntaxGraph::object_type::~object_type() noexcept = default;

SyntaxGraph::vertex_type::allocator_type SyntaxGraph::vertex_type::get_allocator() const noexcept {
    return allocator_type{ mOutEdges.get_allocator().resource() };
}

SyntaxGraph::vertex_type::vertex_type(const allocator_type& alloc)
    : mOutEdges(alloc)
    , mInEdges(alloc) {}

SyntaxGraph::vertex_type::vertex_type(vertex_type&& rhs, const allocator_type& alloc)
    : mOutEdges(std::move(rhs.mOutEdges), alloc)
    , mInEdges(std::move(rhs.mInEdges), alloc)
    , mHandle(std::move(rhs.mHandle)) {}

SyntaxGraph::vertex_type::vertex_type(const vertex_type& rhs, const allocator_type& alloc)
    : mOutEdges(rhs.mOutEdges, alloc)
    , mInEdges(rhs.mInEdges, alloc)
    , mHandle(rhs.mHandle) {}

SyntaxGraph::vertex_type::vertex_type(vertex_type&&) noexcept = default;
SyntaxGraph::vertex_type& SyntaxGraph::vertex_type::operator=(vertex_type&&) = default;
SyntaxGraph::vertex_type& SyntaxGraph::vertex_type::operator=(vertex_type const&) = default;
SyntaxGraph::vertex_type::~vertex_type() noexcept = default;

// ContinuousContainer
void SyntaxGraph::reserve(vertices_size_type sz) {
    mObjects.reserve(sz);
    mVertices.reserve(sz);
    mNames.reserve(sz);
    mTraits.reserve(sz);
    mModulePaths.reserve(sz);
    mTypescripts.reserve(sz);
}

ModuleGraph::allocator_type ModuleGraph::get_allocator() const noexcept {
    return allocator_type(mVertices.get_allocator().resource());
}

ModuleGraph::ModuleGraph(const allocator_type& alloc)
    : mObjects(alloc)
    , mVertices(alloc)
    , mNames(alloc)
    , mModules(alloc)
    , mPathIndex(alloc) {}

ModuleGraph::ModuleGraph(ModuleGraph&& rhs, const allocator_type& alloc)
    : mObjects(std::move(rhs.mObjects), alloc)
    , mVertices(std::move(rhs.mVertices), alloc)
    , mNames(std::move(rhs.mNames), alloc)
    , mModules(std::move(rhs.mModules), alloc)
    , mPathIndex(std::move(rhs.mPathIndex), alloc) {}

ModuleGraph::ModuleGraph(ModuleGraph const& rhs, const allocator_type& alloc)
    : mObjects(rhs.mObjects, alloc)
    , mVertices(rhs.mVertices, alloc)
    , mNames(rhs.mNames, alloc)
    , mModules(rhs.mModules, alloc)
    , mPathIndex(rhs.mPathIndex, alloc) {}

ModuleGraph::ModuleGraph(ModuleGraph&& rhs) = default;
ModuleGraph::~ModuleGraph() noexcept = default;
ModuleGraph& ModuleGraph::operator=(ModuleGraph&& rhs) = default;
ModuleGraph& ModuleGraph::operator=(ModuleGraph const& rhs) = default;

ModuleGraph::object_type::allocator_type ModuleGraph::object_type::get_allocator() const noexcept {
    return allocator_type{ mChildren.get_allocator().resource() };
}

ModuleGraph::object_type::object_type(const allocator_type& alloc)
    : mChildren(alloc)
    , mParents(alloc) {}

ModuleGraph::object_type::object_type(object_type&& rhs, const allocator_type& alloc)
    : mChildren(std::move(rhs.mChildren), alloc)
    , mParents(std::move(rhs.mParents), alloc) {}

ModuleGraph::object_type::object_type(const object_type& rhs, const allocator_type& alloc)
    : mChildren(rhs.mChildren, alloc)
    , mParents(rhs.mParents, alloc) {}

ModuleGraph::object_type::object_type(object_type&&) noexcept = default;
ModuleGraph::object_type& ModuleGraph::object_type::operator=(object_type&&) = default;
ModuleGraph::object_type& ModuleGraph::object_type::operator=(object_type const&) = default;
ModuleGraph::object_type::~object_type() noexcept = default;

ModuleGraph::vertex_type::allocator_type ModuleGraph::vertex_type::get_allocator() const noexcept {
    return allocator_type{ mOutEdges.get_allocator().resource() };
}

ModuleGraph::vertex_type::vertex_type(const allocator_type& alloc)
    : mOutEdges(alloc)
    , mInEdges(alloc) {}

ModuleGraph::vertex_type::vertex_type(vertex_type&& rhs, const allocator_type& alloc)
    : mOutEdges(std::move(rhs.mOutEdges), alloc)
    , mInEdges(std::move(rhs.mInEdges), alloc) {}

ModuleGraph::vertex_type::vertex_type(const vertex_type& rhs, const allocator_type& alloc)
    : mOutEdges(rhs.mOutEdges, alloc)
    , mInEdges(rhs.mInEdges, alloc) {}

ModuleGraph::vertex_type::vertex_type(vertex_type&&) noexcept = default;
ModuleGraph::vertex_type& ModuleGraph::vertex_type::operator=(vertex_type&&) = default;
ModuleGraph::vertex_type& ModuleGraph::vertex_type::operator=(vertex_type const&) = default;
ModuleGraph::vertex_type::~vertex_type() noexcept = default;

// ContinuousContainer
void ModuleGraph::reserve(vertices_size_type sz) {
    mObjects.reserve(sz);
    mVertices.reserve(sz);
    mNames.reserve(sz);
    mModules.reserve(sz);
}

} // namespace Meta

} // namespace Cocos
