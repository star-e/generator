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

#include "SyntaxTypes.h"

namespace Cocos {

namespace Meta {

Instance::Instance(const allocator_type& alloc) noexcept
    : mTemplate(alloc)
    , mParameters(alloc) {}

Instance::Instance(Instance&& rhs, const allocator_type& alloc)
    : mTemplate(std::move(rhs.mTemplate), alloc)
    , mParameters(std::move(rhs.mParameters), alloc) {}

Instance::Instance(Instance const& rhs, const allocator_type& alloc)
    : mTemplate(rhs.mTemplate, alloc)
    , mParameters(rhs.mParameters, alloc) {}

Instance::~Instance() noexcept = default;

Define::Define(const allocator_type& alloc)
    : mContent(alloc) {}

Define::Define(std::string_view content, const allocator_type& alloc)
    : mContent(std::move(content), alloc) {}

Define::Define(Define&& rhs, const allocator_type& alloc)
    : mContent(std::move(rhs.mContent), alloc) {}

Define::Define(Define const& rhs, const allocator_type& alloc)
    : mContent(rhs.mContent, alloc) {}

Define::~Define() noexcept = default;

Concept::Concept(const allocator_type& alloc)
    : mParentPath(alloc) {}

Concept::Concept(std::string_view parentPath, const allocator_type& alloc)
    : mParentPath(std::move(parentPath), alloc) {}

Concept::Concept(Concept&& rhs, const allocator_type& alloc)
    : mParentPath(std::move(rhs.mParentPath), alloc) {}

Concept::Concept(Concept const& rhs, const allocator_type& alloc)
    : mParentPath(rhs.mParentPath, alloc) {}

Concept::~Concept() noexcept = default;

Constraints::Constraints(const allocator_type& alloc) noexcept
    : mConcepts(alloc) {}

Constraints::Constraints(Constraints&& rhs, const allocator_type& alloc)
    : mConcepts(std::move(rhs.mConcepts), alloc) {}

Constraints::Constraints(Constraints const& rhs, const allocator_type& alloc)
    : mConcepts(rhs.mConcepts, alloc) {}

Constraints::~Constraints() noexcept = default;

Base::Base(const allocator_type& alloc)
    : mTypePath(alloc) {}

Base::Base(std::string_view typePath, const allocator_type& alloc)
    : mTypePath(std::move(typePath), alloc) {}

Base::Base(std::string_view typePath, bool virtualBase, const allocator_type& alloc)
    : mTypePath(std::move(typePath), alloc)
    , mVirtualBase(std::move(virtualBase)) {}

Base::Base(std::string_view typePath, bool virtualBase, bool implements, const allocator_type& alloc)
    : mTypePath(std::move(typePath), alloc)
    , mVirtualBase(std::move(virtualBase))
    , mImplements(std::move(implements)) {}

Base::Base(Base&& rhs, const allocator_type& alloc)
    : mTypePath(std::move(rhs.mTypePath), alloc)
    , mVirtualBase(std::move(rhs.mVirtualBase))
    , mImplements(std::move(rhs.mImplements)) {}

Base::Base(Base const& rhs, const allocator_type& alloc)
    : mTypePath(rhs.mTypePath, alloc)
    , mVirtualBase(rhs.mVirtualBase)
    , mImplements(rhs.mImplements) {}

Base::~Base() noexcept = default;

Inherits::Inherits(const allocator_type& alloc) noexcept
    : mBases(alloc) {}

Inherits::Inherits(Inherits&& rhs, const allocator_type& alloc)
    : mBases(std::move(rhs.mBases), alloc) {}

Inherits::Inherits(Inherits const& rhs, const allocator_type& alloc)
    : mBases(rhs.mBases, alloc) {}

Inherits::~Inherits() noexcept = default;

Alias::Alias(const allocator_type& alloc)
    : mTypePath(alloc) {}

Alias::Alias(std::string_view typePath, const allocator_type& alloc)
    : mTypePath(std::move(typePath), alloc) {}

Alias::Alias(Alias&& rhs, const allocator_type& alloc)
    : mTypePath(std::move(rhs.mTypePath), alloc) {}

Alias::Alias(Alias const& rhs, const allocator_type& alloc)
    : mTypePath(rhs.mTypePath, alloc) {}

Alias::~Alias() noexcept = default;

EnumValue::EnumValue(const allocator_type& alloc) noexcept
    : mName(alloc)
    , mValue(alloc)
    , mReflectionName(alloc)
    , mComment(alloc) {}

EnumValue::EnumValue(EnumValue&& rhs, const allocator_type& alloc)
    : mName(std::move(rhs.mName), alloc)
    , mValue(std::move(rhs.mValue), alloc)
    , mReflectionName(std::move(rhs.mReflectionName), alloc)
    , mComment(std::move(rhs.mComment), alloc)
    , mExport(std::move(rhs.mExport))
    , mAlias(std::move(rhs.mAlias)) {}

EnumValue::EnumValue(EnumValue const& rhs, const allocator_type& alloc)
    : mName(rhs.mName, alloc)
    , mValue(rhs.mValue, alloc)
    , mReflectionName(rhs.mReflectionName, alloc)
    , mComment(rhs.mComment, alloc)
    , mExport(rhs.mExport)
    , mAlias(rhs.mAlias) {}

EnumValue::~EnumValue() noexcept = default;

Enum::Enum(const allocator_type& alloc) noexcept
    : mUnderlyingType(alloc)
    , mValues(alloc) {}

Enum::Enum(Enum&& rhs, const allocator_type& alloc)
    : mIsFlags(std::move(rhs.mIsFlags))
    , mEnumOperator(std::move(rhs.mEnumOperator))
    , mHasName(std::move(rhs.mHasName))
    , mUnderlyingType(std::move(rhs.mUnderlyingType), alloc)
    , mValues(std::move(rhs.mValues), alloc) {}

Enum::Enum(Enum const& rhs, const allocator_type& alloc)
    : mIsFlags(rhs.mIsFlags)
    , mEnumOperator(rhs.mEnumOperator)
    , mHasName(rhs.mHasName)
    , mUnderlyingType(rhs.mUnderlyingType, alloc)
    , mValues(rhs.mValues, alloc) {}

Enum::~Enum() noexcept = default;

Member::Member(const allocator_type& alloc) noexcept
    : mTypePath(alloc)
    , mMemberName(alloc)
    , mDefaultValue(alloc)
    , mComments(alloc)
    , mTypescriptType(alloc)
    , mTypescriptDefaultValue(alloc) {}

Member::Member(Member&& rhs, const allocator_type& alloc)
    : mTypePath(std::move(rhs.mTypePath), alloc)
    , mMemberName(std::move(rhs.mMemberName), alloc)
    , mDefaultValue(std::move(rhs.mDefaultValue), alloc)
    , mComments(std::move(rhs.mComments), alloc)
    , mConst(std::move(rhs.mConst))
    , mMutable(std::move(rhs.mMutable))
    , mPointer(std::move(rhs.mPointer))
    , mReference(std::move(rhs.mReference))
    , mPublic(std::move(rhs.mPublic))
    , mOptional(std::move(rhs.mOptional))
    , mFlags(std::move(rhs.mFlags))
    , mTypescriptType(std::move(rhs.mTypescriptType), alloc)
    , mTypescriptDefaultValue(std::move(rhs.mTypescriptDefaultValue), alloc)
    , mTypescriptArray(std::move(rhs.mTypescriptArray))
    , mTypescriptNullable(std::move(rhs.mTypescriptNullable)) {}

Member::Member(Member const& rhs, const allocator_type& alloc)
    : mTypePath(rhs.mTypePath, alloc)
    , mMemberName(rhs.mMemberName, alloc)
    , mDefaultValue(rhs.mDefaultValue, alloc)
    , mComments(rhs.mComments, alloc)
    , mConst(rhs.mConst)
    , mMutable(rhs.mMutable)
    , mPointer(rhs.mPointer)
    , mReference(rhs.mReference)
    , mPublic(rhs.mPublic)
    , mOptional(rhs.mOptional)
    , mFlags(rhs.mFlags)
    , mTypescriptType(rhs.mTypescriptType, alloc)
    , mTypescriptDefaultValue(rhs.mTypescriptDefaultValue, alloc)
    , mTypescriptArray(rhs.mTypescriptArray)
    , mTypescriptNullable(rhs.mTypescriptNullable) {}

Member::~Member() noexcept = default;

Parameter::Parameter(const allocator_type& alloc) noexcept
    : mTypePath(alloc)
    , mName(alloc)
    , mDefaultValue(alloc)
    , mComment(alloc) {}

Parameter::Parameter(Parameter&& rhs, const allocator_type& alloc)
    : mTypePath(std::move(rhs.mTypePath), alloc)
    , mName(std::move(rhs.mName), alloc)
    , mDefaultValue(std::move(rhs.mDefaultValue), alloc)
    , mComment(std::move(rhs.mComment), alloc)
    , mConst(std::move(rhs.mConst))
    , mPointer(std::move(rhs.mPointer))
    , mReference(std::move(rhs.mReference))
    , mRvalue(std::move(rhs.mRvalue))
    , mOptional(std::move(rhs.mOptional))
    , mRealPointer(std::move(rhs.mRealPointer)) {}

Parameter::Parameter(Parameter const& rhs, const allocator_type& alloc)
    : mTypePath(rhs.mTypePath, alloc)
    , mName(rhs.mName, alloc)
    , mDefaultValue(rhs.mDefaultValue, alloc)
    , mComment(rhs.mComment, alloc)
    , mConst(rhs.mConst)
    , mPointer(rhs.mPointer)
    , mReference(rhs.mReference)
    , mRvalue(rhs.mRvalue)
    , mOptional(rhs.mOptional)
    , mRealPointer(rhs.mRealPointer) {}

Parameter::~Parameter() noexcept = default;

Method::Method(const allocator_type& alloc) noexcept
    : mReturnType(alloc)
    , mFunctionName(alloc)
    , mParameters(alloc)
    , mComment(alloc) {}

Method::Method(Method&& rhs, const allocator_type& alloc)
    : mReturnType(std::move(rhs.mReturnType), alloc)
    , mFunctionName(std::move(rhs.mFunctionName), alloc)
    , mParameters(std::move(rhs.mParameters), alloc)
    , mComment(std::move(rhs.mComment), alloc)
    , mVirtual(std::move(rhs.mVirtual))
    , mConst(std::move(rhs.mConst))
    , mNoexcept(std::move(rhs.mNoexcept))
    , mPure(std::move(rhs.mPure))
    , mStatic(std::move(rhs.mStatic))
    , mGetter(std::move(rhs.mGetter))
    , mSetter(std::move(rhs.mSetter))
    , mOptional(std::move(rhs.mOptional))
    , mOptionalMethod(std::move(rhs.mOptionalMethod))
    , mSkip(std::move(rhs.mSkip))
    , mCovariant(std::move(rhs.mCovariant))
    , mFlags(std::move(rhs.mFlags)) {}

Method::Method(Method const& rhs, const allocator_type& alloc)
    : mReturnType(rhs.mReturnType, alloc)
    , mFunctionName(rhs.mFunctionName, alloc)
    , mParameters(rhs.mParameters, alloc)
    , mComment(rhs.mComment, alloc)
    , mVirtual(rhs.mVirtual)
    , mConst(rhs.mConst)
    , mNoexcept(rhs.mNoexcept)
    , mPure(rhs.mPure)
    , mStatic(rhs.mStatic)
    , mGetter(rhs.mGetter)
    , mSetter(rhs.mSetter)
    , mOptional(rhs.mOptional)
    , mOptionalMethod(rhs.mOptionalMethod)
    , mSkip(rhs.mSkip)
    , mCovariant(rhs.mCovariant)
    , mFlags(rhs.mFlags) {}

Method::~Method() noexcept = default;

Constructor::Constructor(const allocator_type& alloc) noexcept
    : mIndices(alloc) {}

Constructor::Constructor(Constructor&& rhs, const allocator_type& alloc)
    : mIndices(std::move(rhs.mIndices), alloc)
    , mHasDefault(std::move(rhs.mHasDefault)) {}

Constructor::Constructor(Constructor const& rhs, const allocator_type& alloc)
    : mIndices(rhs.mIndices, alloc)
    , mHasDefault(rhs.mHasDefault) {}

Constructor::~Constructor() noexcept = default;

BaseConstructor::BaseConstructor(const allocator_type& alloc) noexcept
    : mParameters(alloc) {}

BaseConstructor::BaseConstructor(BaseConstructor&& rhs, const allocator_type& alloc)
    : mParameters(std::move(rhs.mParameters), alloc)
    , mBaseID(std::move(rhs.mBaseID)) {}

BaseConstructor::BaseConstructor(BaseConstructor const& rhs, const allocator_type& alloc)
    : mParameters(rhs.mParameters, alloc)
    , mBaseID(rhs.mBaseID) {}

BaseConstructor::~BaseConstructor() noexcept = default;

Struct::Struct(const allocator_type& alloc) noexcept
    : mMembers(alloc)
    , mConstructors(alloc)
    , mMemberFunctions(alloc)
    , mTypescriptMembers(alloc)
    , mTypescriptFunctions(alloc)
    , mMethods(alloc) {}

Struct::Struct(Struct&& rhs, const allocator_type& alloc)
    : mMembers(std::move(rhs.mMembers), alloc)
    , mConstructors(std::move(rhs.mConstructors), alloc)
    , mMemberFunctions(std::move(rhs.mMemberFunctions), alloc)
    , mTypescriptMembers(std::move(rhs.mTypescriptMembers), alloc)
    , mTypescriptFunctions(std::move(rhs.mTypescriptFunctions), alloc)
    , mMethods(std::move(rhs.mMethods), alloc) {}

Struct::Struct(Struct const& rhs, const allocator_type& alloc)
    : mMembers(rhs.mMembers, alloc)
    , mConstructors(rhs.mConstructors, alloc)
    , mMemberFunctions(rhs.mMemberFunctions, alloc)
    , mTypescriptMembers(rhs.mTypescriptMembers, alloc)
    , mTypescriptFunctions(rhs.mTypescriptFunctions, alloc)
    , mMethods(rhs.mMethods, alloc) {}

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
    , mMemberName(alloc)
    , mContainerPath(alloc) {}

PolymorphicPair::PolymorphicPair(PolymorphicPair&& rhs, const allocator_type& alloc)
    : mTag(std::move(rhs.mTag), alloc)
    , mValue(std::move(rhs.mValue), alloc)
    , mMemberName(std::move(rhs.mMemberName), alloc)
    , mContainerPath(std::move(rhs.mContainerPath), alloc)
    , mVector(std::move(rhs.mVector)) {}

PolymorphicPair::PolymorphicPair(PolymorphicPair const& rhs, const allocator_type& alloc)
    : mTag(rhs.mTag, alloc)
    , mValue(rhs.mValue, alloc)
    , mMemberName(rhs.mMemberName, alloc)
    , mContainerPath(rhs.mContainerPath, alloc)
    , mVector(rhs.mVector) {}

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
    , mMemberName(alloc)
    , mContainerPath(alloc) {}

Component::Component(Component&& rhs, const allocator_type& alloc)
    : mName(std::move(rhs.mName), alloc)
    , mValuePath(std::move(rhs.mValuePath), alloc)
    , mMemberName(std::move(rhs.mMemberName), alloc)
    , mContainerPath(std::move(rhs.mContainerPath), alloc)
    , mVector(std::move(rhs.mVector)) {}

Component::Component(Component const& rhs, const allocator_type& alloc)
    : mName(rhs.mName, alloc)
    , mValuePath(rhs.mValuePath, alloc)
    , mMemberName(rhs.mMemberName, alloc)
    , mContainerPath(rhs.mContainerPath, alloc)
    , mVector(rhs.mVector) {}

Component::~Component() noexcept = default;

VertexMap::VertexMap(const allocator_type& alloc) noexcept
    : mMapType(alloc)
    , mMemberName(alloc)
    , mKeyType(alloc)
    , mComponentName(alloc)
    , mComponentMemberName(alloc)
    , mTypePath(alloc) {}

VertexMap::VertexMap(VertexMap&& rhs, const allocator_type& alloc)
    : mMapType(std::move(rhs.mMapType), alloc)
    , mMemberName(std::move(rhs.mMemberName), alloc)
    , mKeyType(std::move(rhs.mKeyType), alloc)
    , mComponentName(std::move(rhs.mComponentName), alloc)
    , mComponentMemberName(std::move(rhs.mComponentMemberName), alloc)
    , mTypePath(std::move(rhs.mTypePath), alloc) {}

VertexMap::VertexMap(VertexMap const& rhs, const allocator_type& alloc)
    : mMapType(rhs.mMapType, alloc)
    , mMemberName(rhs.mMemberName, alloc)
    , mKeyType(rhs.mKeyType, alloc)
    , mComponentName(rhs.mComponentName, alloc)
    , mComponentMemberName(rhs.mComponentMemberName, alloc)
    , mTypePath(rhs.mTypePath, alloc) {}

VertexMap::~VertexMap() noexcept = default;

Layer::Layer(const allocator_type& alloc) noexcept
    : mContainer(alloc)
    , mMemberName(alloc)
    , mGraphPath(alloc)
    , mTagPath(alloc) {}

Layer::Layer(Layer&& rhs, const allocator_type& alloc)
    : mContainer(std::move(rhs.mContainer), alloc)
    , mMemberName(std::move(rhs.mMemberName), alloc)
    , mGraphPath(std::move(rhs.mGraphPath), alloc)
    , mTagPath(std::move(rhs.mTagPath), alloc)
    , mContainerType(std::move(rhs.mContainerType)) {}

Layer::Layer(Layer const& rhs, const allocator_type& alloc)
    : mContainer(rhs.mContainer, alloc)
    , mMemberName(rhs.mMemberName, alloc)
    , mGraphPath(rhs.mGraphPath, alloc)
    , mTagPath(rhs.mTagPath, alloc)
    , mContainerType(rhs.mContainerType) {}

Layer::~Layer() noexcept = default;

Stack::Stack(const allocator_type& alloc) noexcept
    : mLayers(alloc)
    , mContainer(alloc) {}

Stack::Stack(Stack&& rhs, const allocator_type& alloc)
    : mLayers(std::move(rhs.mLayers), alloc)
    , mContainer(std::move(rhs.mContainer), alloc) {}

Stack::Stack(Stack const& rhs, const allocator_type& alloc)
    : mLayers(rhs.mLayers, alloc)
    , mContainer(rhs.mContainer, alloc) {}

Stack::~Stack() noexcept = default;

Named::Named(const allocator_type& alloc) noexcept
    : mComponentName(alloc)
    , mComponentMemberName(alloc) {}

Named::Named(Named&& rhs, const allocator_type& alloc)
    : mComponent(std::move(rhs.mComponent))
    , mComponentName(std::move(rhs.mComponentName), alloc)
    , mComponentMemberName(std::move(rhs.mComponentMemberName), alloc) {}

Named::Named(Named const& rhs, const allocator_type& alloc)
    : mComponent(rhs.mComponent)
    , mComponentName(rhs.mComponentName, alloc)
    , mComponentMemberName(rhs.mComponentMemberName, alloc) {}

Named::~Named() noexcept = default;

Addressable::Addressable(const allocator_type& alloc) noexcept
    : mMemberName(alloc) {}

Addressable::Addressable(Addressable&& rhs, const allocator_type& alloc)
    : mUtf8(std::move(rhs.mUtf8))
    , mPathPropertyMap(std::move(rhs.mPathPropertyMap))
    , mType(std::move(rhs.mType))
    , mMemberName(std::move(rhs.mMemberName), alloc) {}

Addressable::Addressable(Addressable const& rhs, const allocator_type& alloc)
    : mUtf8(rhs.mUtf8)
    , mPathPropertyMap(rhs.mPathPropertyMap)
    , mType(rhs.mType)
    , mMemberName(rhs.mMemberName, alloc) {}

Addressable::~Addressable() noexcept = default;

MemberRecord::MemberRecord(const allocator_type& alloc) noexcept
    : mComment(alloc)
    , mType(alloc)
    , mMember(alloc)
    , mDefaultValue(alloc) {}

MemberRecord::MemberRecord(MemberRecord&& rhs, const allocator_type& alloc)
    : mComment(std::move(rhs.mComment), alloc)
    , mType(std::move(rhs.mType), alloc)
    , mMember(std::move(rhs.mMember), alloc)
    , mDefaultValue(std::move(rhs.mDefaultValue), alloc)
    , mDefaultValueOffset(std::move(rhs.mDefaultValueOffset)) {}

MemberRecord::MemberRecord(MemberRecord const& rhs, const allocator_type& alloc)
    : mComment(rhs.mComment, alloc)
    , mType(rhs.mType, alloc)
    , mMember(rhs.mMember, alloc)
    , mDefaultValue(rhs.mDefaultValue, alloc)
    , mDefaultValueOffset(rhs.mDefaultValueOffset) {}

MemberRecord::~MemberRecord() noexcept = default;

MemberFormatter::MemberFormatter(const allocator_type& alloc) noexcept
    : mMembers(alloc) {}

MemberFormatter::MemberFormatter(MemberFormatter&& rhs, const allocator_type& alloc)
    : mMembers(std::move(rhs.mMembers), alloc)
    , mTypeLength(std::move(rhs.mTypeLength)) {}

MemberFormatter::MemberFormatter(MemberFormatter const& rhs, const allocator_type& alloc)
    : mMembers(rhs.mMembers, alloc)
    , mTypeLength(rhs.mTypeLength) {}

MemberFormatter::~MemberFormatter() noexcept = default;

Graph::Graph(const allocator_type& alloc) noexcept
    : mMembers(alloc)
    , mConstructors(alloc)
    , mMemberFunctions(alloc)
    , mMethods(alloc)
    , mVertexProperty(alloc)
    , mEdgeProperty(alloc)
    , mComponents(alloc)
    , mPolymorphic(alloc)
    , mVertexMaps(alloc)
    , mNamedConcept(alloc)
    , mAddressableConcept(alloc)
    , mTypescriptMembers(alloc)
    , mTypescriptFunctions(alloc) {}

Graph::Graph(Graph&& rhs, const allocator_type& alloc)
    : mMembers(std::move(rhs.mMembers), alloc)
    , mConstructors(std::move(rhs.mConstructors), alloc)
    , mMemberFunctions(std::move(rhs.mMemberFunctions), alloc)
    , mMethods(std::move(rhs.mMethods), alloc)
    , mVertexProperty(std::move(rhs.mVertexProperty), alloc)
    , mEdgeProperty(std::move(rhs.mEdgeProperty), alloc)
    , mComponents(std::move(rhs.mComponents), alloc)
    , mIncidence(std::move(rhs.mIncidence))
    , mAdjacency(std::move(rhs.mAdjacency))
    , mUndirected(std::move(rhs.mUndirected))
    , mBidirectional(std::move(rhs.mBidirectional))
    , mVertexList(std::move(rhs.mVertexList))
    , mEdgeList(std::move(rhs.mEdgeList))
    , mMutableGraphVertex(std::move(rhs.mMutableGraphVertex))
    , mMutableGraphEdge(std::move(rhs.mMutableGraphEdge))
    , mNamed(std::move(rhs.mNamed))
    , mReferenceGraph(std::move(rhs.mReferenceGraph))
    , mAliasGraph(std::move(rhs.mAliasGraph))
    , mAddressable(std::move(rhs.mAddressable))
    , mAddressIndex(std::move(rhs.mAddressIndex))
    , mMutableReference(std::move(rhs.mMutableReference))
    , mColorMap(std::move(rhs.mColorMap))
    , mPolymorphic(std::move(rhs.mPolymorphic), alloc)
    , mVertexListType(std::move(rhs.mVertexListType))
    , mEdgeListType(std::move(rhs.mEdgeListType))
    , mOutEdgeListType(std::move(rhs.mOutEdgeListType))
    , mVertexListPath(std::move(rhs.mVertexListPath))
    , mEdgeListPath(std::move(rhs.mEdgeListPath))
    , mOutEdgeListPath(std::move(rhs.mOutEdgeListPath))
    , mVertexDescriptor(std::move(rhs.mVertexDescriptor))
    , mVertexSizeType(std::move(rhs.mVertexSizeType))
    , mDifferenceType(std::move(rhs.mDifferenceType))
    , mEdgeSizeType(std::move(rhs.mEdgeSizeType))
    , mEdgeDifferenceType(std::move(rhs.mEdgeDifferenceType))
    , mDegreeSizeType(std::move(rhs.mDegreeSizeType))
    , mVertexMaps(std::move(rhs.mVertexMaps), alloc)
    , mNamedConcept(std::move(rhs.mNamedConcept), alloc)
    , mAddressableConcept(std::move(rhs.mAddressableConcept), alloc)
    , mTypescriptMembers(std::move(rhs.mTypescriptMembers), alloc)
    , mTypescriptFunctions(std::move(rhs.mTypescriptFunctions), alloc) {}

Graph::Graph(Graph const& rhs, const allocator_type& alloc)
    : mMembers(rhs.mMembers, alloc)
    , mConstructors(rhs.mConstructors, alloc)
    , mMemberFunctions(rhs.mMemberFunctions, alloc)
    , mMethods(rhs.mMethods, alloc)
    , mVertexProperty(rhs.mVertexProperty, alloc)
    , mEdgeProperty(rhs.mEdgeProperty, alloc)
    , mComponents(rhs.mComponents, alloc)
    , mIncidence(rhs.mIncidence)
    , mAdjacency(rhs.mAdjacency)
    , mUndirected(rhs.mUndirected)
    , mBidirectional(rhs.mBidirectional)
    , mVertexList(rhs.mVertexList)
    , mEdgeList(rhs.mEdgeList)
    , mMutableGraphVertex(rhs.mMutableGraphVertex)
    , mMutableGraphEdge(rhs.mMutableGraphEdge)
    , mNamed(rhs.mNamed)
    , mReferenceGraph(rhs.mReferenceGraph)
    , mAliasGraph(rhs.mAliasGraph)
    , mAddressable(rhs.mAddressable)
    , mAddressIndex(rhs.mAddressIndex)
    , mMutableReference(rhs.mMutableReference)
    , mColorMap(rhs.mColorMap)
    , mPolymorphic(rhs.mPolymorphic, alloc)
    , mVertexListType(rhs.mVertexListType)
    , mEdgeListType(rhs.mEdgeListType)
    , mOutEdgeListType(rhs.mOutEdgeListType)
    , mVertexListPath(rhs.mVertexListPath)
    , mEdgeListPath(rhs.mEdgeListPath)
    , mOutEdgeListPath(rhs.mOutEdgeListPath)
    , mVertexDescriptor(rhs.mVertexDescriptor)
    , mVertexSizeType(rhs.mVertexSizeType)
    , mDifferenceType(rhs.mDifferenceType)
    , mEdgeSizeType(rhs.mEdgeSizeType)
    , mEdgeDifferenceType(rhs.mEdgeDifferenceType)
    , mDegreeSizeType(rhs.mDegreeSizeType)
    , mVertexMaps(rhs.mVertexMaps, alloc)
    , mNamedConcept(rhs.mNamedConcept, alloc)
    , mAddressableConcept(rhs.mAddressableConcept, alloc)
    , mTypescriptMembers(rhs.mTypescriptMembers, alloc)
    , mTypescriptFunctions(rhs.mTypescriptFunctions, alloc) {}

Graph::~Graph() noexcept = default;

Typescript::Typescript(const allocator_type& alloc) noexcept
    : mName(alloc) {}

Typescript::Typescript(Typescript&& rhs, const allocator_type& alloc)
    : mName(std::move(rhs.mName), alloc)
    , mArray(std::move(rhs.mArray)) {}

Typescript::Typescript(Typescript const& rhs, const allocator_type& alloc)
    : mName(rhs.mName, alloc)
    , mArray(rhs.mArray) {}

Typescript::~Typescript() noexcept = default;

Comment::Comment(const allocator_type& alloc)
    : mComment(alloc)
    , mParameterComments(alloc) {}

Comment::Comment(Comment&& rhs, const allocator_type& alloc)
    : mComment(std::move(rhs.mComment), alloc)
    , mParameterComments(std::move(rhs.mParameterComments), alloc) {}

Comment::Comment(Comment const& rhs, const allocator_type& alloc)
    : mComment(rhs.mComment, alloc)
    , mParameterComments(rhs.mParameterComments, alloc) {}

Comment::~Comment() noexcept = default;

SyntaxGraph::allocator_type SyntaxGraph::get_allocator() const noexcept {
    return allocator_type(mVertices.get_allocator().resource());
}

SyntaxGraph::SyntaxGraph(const allocator_type& alloc)
    : mObjects(alloc)
    , mVertices(alloc)
    , mNames(alloc)
    , mTraits(alloc)
    , mConstraints(alloc)
    , mInherits(alloc)
    , mModulePaths(alloc)
    , mTypescripts(alloc)
    , mComments(alloc)
    , mDefines(alloc)
    , mConcepts(alloc)
    , mAliases(alloc)
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
    , mConstraints(std::move(rhs.mConstraints), alloc)
    , mInherits(std::move(rhs.mInherits), alloc)
    , mModulePaths(std::move(rhs.mModulePaths), alloc)
    , mTypescripts(std::move(rhs.mTypescripts), alloc)
    , mComments(std::move(rhs.mComments), alloc)
    , mDefines(std::move(rhs.mDefines), alloc)
    , mConcepts(std::move(rhs.mConcepts), alloc)
    , mAliases(std::move(rhs.mAliases), alloc)
    , mEnums(std::move(rhs.mEnums), alloc)
    , mTags(std::move(rhs.mTags), alloc)
    , mStructs(std::move(rhs.mStructs), alloc)
    , mGraphs(std::move(rhs.mGraphs), alloc)
    , mVariants(std::move(rhs.mVariants), alloc)
    , mInstances(std::move(rhs.mInstances), alloc)
    , mScratch(std::move(rhs.mScratch))
    , mPathIndex(std::move(rhs.mPathIndex), alloc) {}

SyntaxGraph::SyntaxGraph(SyntaxGraph const& rhs, const allocator_type& alloc)
    : mObjects(rhs.mObjects, alloc)
    , mVertices(rhs.mVertices, alloc)
    , mNames(rhs.mNames, alloc)
    , mTraits(rhs.mTraits, alloc)
    , mConstraints(rhs.mConstraints, alloc)
    , mInherits(rhs.mInherits, alloc)
    , mModulePaths(rhs.mModulePaths, alloc)
    , mTypescripts(rhs.mTypescripts, alloc)
    , mComments(rhs.mComments, alloc)
    , mDefines(rhs.mDefines, alloc)
    , mConcepts(rhs.mConcepts, alloc)
    , mAliases(rhs.mAliases, alloc)
    , mEnums(rhs.mEnums, alloc)
    , mTags(rhs.mTags, alloc)
    , mStructs(rhs.mStructs, alloc)
    , mGraphs(rhs.mGraphs, alloc)
    , mVariants(rhs.mVariants, alloc)
    , mInstances(rhs.mInstances, alloc)
    , mScratch(rhs.mScratch)
    , mPathIndex(rhs.mPathIndex, alloc) {}

SyntaxGraph::SyntaxGraph(SyntaxGraph&& rhs) = default;
SyntaxGraph& SyntaxGraph::operator=(SyntaxGraph&& rhs) = default;
SyntaxGraph& SyntaxGraph::operator=(SyntaxGraph const& rhs) = default;
SyntaxGraph::~SyntaxGraph() noexcept = default;

// ContinuousContainer
void SyntaxGraph::reserve(vertices_size_type sz) {
    mObjects.reserve(sz);
    mVertices.reserve(sz);
    mNames.reserve(sz);
    mTraits.reserve(sz);
    mConstraints.reserve(sz);
    mInherits.reserve(sz);
    mModulePaths.reserve(sz);
    mTypescripts.reserve(sz);
    mComments.reserve(sz);
}

SyntaxGraph::object_type::allocator_type SyntaxGraph::object_type::get_allocator() const noexcept {
    return allocator_type(mChildren.get_allocator().resource());
}

SyntaxGraph::object_type::object_type(const allocator_type& alloc) noexcept
    : mChildren(alloc)
    , mParents(alloc) {}

SyntaxGraph::object_type::object_type(object_type&& rhs, const allocator_type& alloc)
    : mChildren(std::move(rhs.mChildren), alloc)
    , mParents(std::move(rhs.mParents), alloc) {}

SyntaxGraph::object_type::object_type(const object_type& rhs, const allocator_type& alloc)
    : mChildren(rhs.mChildren, alloc)
    , mParents(rhs.mParents, alloc) {}

SyntaxGraph::object_type::object_type(object_type&& rhs) noexcept = default;
SyntaxGraph::object_type& SyntaxGraph::object_type::operator=(object_type&& rhs) = default;
SyntaxGraph::object_type& SyntaxGraph::object_type::operator=(object_type const& rhs) = default;
SyntaxGraph::object_type::~object_type() noexcept = default;

SyntaxGraph::vertex_type::allocator_type SyntaxGraph::vertex_type::get_allocator() const noexcept {
    return allocator_type(mOutEdges.get_allocator().resource());
}

SyntaxGraph::vertex_type::vertex_type(const allocator_type& alloc) noexcept
    : mOutEdges(alloc)
    , mInEdges(alloc) {}

SyntaxGraph::vertex_type::vertex_type(vertex_type&& rhs, const allocator_type& alloc)
    : mOutEdges(std::move(rhs.mOutEdges), alloc)
    , mInEdges(std::move(rhs.mInEdges), alloc)
    , mHandle(std::move(rhs.mHandle)) {}

SyntaxGraph::vertex_type::vertex_type(vertex_type const& rhs, const allocator_type& alloc)
    : mOutEdges(rhs.mOutEdges, alloc)
    , mInEdges(rhs.mInEdges, alloc)
    , mHandle(rhs.mHandle) {}

SyntaxGraph::vertex_type::vertex_type(vertex_type&& rhs) noexcept = default;
SyntaxGraph::vertex_type& SyntaxGraph::vertex_type::operator=(vertex_type&& rhs) = default;
SyntaxGraph::vertex_type& SyntaxGraph::vertex_type::operator=(vertex_type const& rhs) = default;
SyntaxGraph::vertex_type::~vertex_type() noexcept = default;

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
ModuleGraph& ModuleGraph::operator=(ModuleGraph&& rhs) = default;
ModuleGraph& ModuleGraph::operator=(ModuleGraph const& rhs) = default;
ModuleGraph::~ModuleGraph() noexcept = default;

// ContinuousContainer
void ModuleGraph::reserve(vertices_size_type sz) {
    mObjects.reserve(sz);
    mVertices.reserve(sz);
    mNames.reserve(sz);
    mModules.reserve(sz);
}

ModuleGraph::object_type::allocator_type ModuleGraph::object_type::get_allocator() const noexcept {
    return allocator_type(mChildren.get_allocator().resource());
}

ModuleGraph::object_type::object_type(const allocator_type& alloc) noexcept
    : mChildren(alloc)
    , mParents(alloc) {}

ModuleGraph::object_type::object_type(object_type&& rhs, const allocator_type& alloc)
    : mChildren(std::move(rhs.mChildren), alloc)
    , mParents(std::move(rhs.mParents), alloc) {}

ModuleGraph::object_type::object_type(const object_type& rhs, const allocator_type& alloc)
    : mChildren(rhs.mChildren, alloc)
    , mParents(rhs.mParents, alloc) {}

ModuleGraph::object_type::object_type(object_type&& rhs) noexcept = default;
ModuleGraph::object_type& ModuleGraph::object_type::operator=(object_type&& rhs) = default;
ModuleGraph::object_type& ModuleGraph::object_type::operator=(object_type const& rhs) = default;
ModuleGraph::object_type::~object_type() noexcept = default;

ModuleGraph::vertex_type::allocator_type ModuleGraph::vertex_type::get_allocator() const noexcept {
    return allocator_type(mOutEdges.get_allocator().resource());
}

ModuleGraph::vertex_type::vertex_type(const allocator_type& alloc) noexcept
    : mOutEdges(alloc)
    , mInEdges(alloc) {}

ModuleGraph::vertex_type::vertex_type(vertex_type&& rhs, const allocator_type& alloc)
    : mOutEdges(std::move(rhs.mOutEdges), alloc)
    , mInEdges(std::move(rhs.mInEdges), alloc) {}

ModuleGraph::vertex_type::vertex_type(vertex_type const& rhs, const allocator_type& alloc)
    : mOutEdges(rhs.mOutEdges, alloc)
    , mInEdges(rhs.mInEdges, alloc) {}

ModuleGraph::vertex_type::vertex_type(vertex_type&& rhs) noexcept = default;
ModuleGraph::vertex_type& ModuleGraph::vertex_type::operator=(vertex_type&& rhs) = default;
ModuleGraph::vertex_type& ModuleGraph::vertex_type::operator=(vertex_type const& rhs) = default;
ModuleGraph::vertex_type::~vertex_type() noexcept = default;

} // namespace Meta

} // namespace Cocos
