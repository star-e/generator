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

#include "BuilderTypes.h"

namespace Cocos {

namespace Meta {

CppStructBuilder::CppStructBuilder(CppStructBuilder&& rhs, const allocator_type& alloc)
    : mSyntaxGraph(std::move(rhs.mSyntaxGraph))
    , mModuleGraph(std::move(rhs.mModuleGraph))
    , mCurrentVertex(std::move(rhs.mCurrentVertex))
    , mCurrentModule(std::move(rhs.mCurrentModule))
    , mCurrentNamespace(std::move(rhs.mCurrentNamespace), alloc)
    , mCurrentPath(std::move(rhs.mCurrentPath), alloc)
    , mName(std::move(rhs.mName), alloc)
    , mAPI(std::move(rhs.mAPI), alloc)
    , mProjectName(std::move(rhs.mProjectName), alloc) {}

CppStructBuilder::CppStructBuilder(CppStructBuilder const& rhs, const allocator_type& alloc)
    : mSyntaxGraph(rhs.mSyntaxGraph)
    , mModuleGraph(rhs.mModuleGraph)
    , mCurrentVertex(rhs.mCurrentVertex)
    , mCurrentModule(rhs.mCurrentModule)
    , mCurrentNamespace(rhs.mCurrentNamespace, alloc)
    , mCurrentPath(rhs.mCurrentPath, alloc)
    , mName(rhs.mName, alloc)
    , mAPI(rhs.mAPI, alloc)
    , mProjectName(rhs.mProjectName, alloc) {}

CppStructBuilder::~CppStructBuilder() noexcept = default;

GraphBuilder::~GraphBuilder() noexcept = default;

CppGraphBuilder::CppGraphBuilder(const allocator_type& alloc) noexcept
    : mStruct(alloc)
    , mVertexType(alloc) {}

CppGraphBuilder::~CppGraphBuilder() noexcept = default;

TypeHandle::TypeHandle(TypeHandle&& rhs, const allocator_type& alloc)
    : mModuleBuilder(std::move(rhs.mModuleBuilder))
    , mPathSuffix(std::move(rhs.mPathSuffix), alloc)
    , mVertexDescriptor(std::move(rhs.mVertexDescriptor)) {}

ModuleHandle::ModuleHandle(ModuleHandle&& rhs, const allocator_type& alloc)
    : mModuleBuilder(std::move(rhs.mModuleBuilder))
    , mModulePath(std::move(rhs.mModulePath), alloc)
    , mVertexDescriptor(std::move(rhs.mVertexDescriptor)) {}

CodegenScope::CodegenScope(std::string_view name, const allocator_type& alloc)
    : mName(std::move(name), alloc) {}

CodegenScope::CodegenScope(CodegenScope&& rhs, const allocator_type& alloc)
    : mName(std::move(rhs.mName), alloc)
    , mCount(std::move(rhs.mCount)) {}

CodegenScope::CodegenScope(CodegenScope const& rhs, const allocator_type& alloc)
    : mName(rhs.mName, alloc)
    , mCount(rhs.mCount) {}

CodegenScope::~CodegenScope() noexcept = default;

CodegenContext::CodegenContext(const allocator_type& alloc) noexcept
    : mScopes(alloc)
    , mNamespaces(alloc) {}

CodegenContext::CodegenContext(CodegenContext&& rhs, const allocator_type& alloc)
    : mScopes(std::move(rhs.mScopes), alloc)
    , mNamespaces(std::move(rhs.mNamespaces), alloc) {}

CodegenContext::CodegenContext(CodegenContext const& rhs, const allocator_type& alloc)
    : mScopes(rhs.mScopes, alloc)
    , mNamespaces(rhs.mNamespaces, alloc) {}

CodegenContext::~CodegenContext() noexcept = default;

ModuleBuilder::ModuleBuilder(const allocator_type& alloc)
    : mSyntaxGraph(alloc)
    , mModuleGraph(alloc)
    , mCurrentModule(alloc)
    , mCurrentScope(alloc)
    , mProjectName(alloc) {}

ModuleBuilder::ModuleBuilder(ModuleBuilder&& rhs, const allocator_type& alloc)
    : mFolder(std::move(rhs.mFolder))
    , mSyntaxGraph(std::move(rhs.mSyntaxGraph), alloc)
    , mModuleGraph(std::move(rhs.mModuleGraph), alloc)
    , mCurrentModule(std::move(rhs.mCurrentModule), alloc)
    , mCurrentScope(std::move(rhs.mCurrentScope), alloc)
    , mProjectName(std::move(rhs.mProjectName), alloc)
    , mScratch(std::move(rhs.mScratch))
    , mUnderscoreMemberName(std::move(rhs.mUnderscoreMemberName))
    , mCompiled(std::move(rhs.mCompiled)) {}

ModuleBuilder::~ModuleBuilder() noexcept = default;

} // namespace Meta

} // namespace Cocos
