#include "BuilderTypes.h"

namespace Cocos {

namespace Meta {

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
    , mCurrentScope(alloc) {}

ModuleBuilder::ModuleBuilder(ModuleBuilder&& rhs, const allocator_type& alloc)
    : mSyntaxGraph(std::move(rhs.mSyntaxGraph), alloc)
    , mModuleGraph(std::move(rhs.mModuleGraph), alloc)
    , mCurrentModule(std::move(rhs.mCurrentModule), alloc)
    , mCurrentScope(std::move(rhs.mCurrentScope), alloc)
    , mScratch(std::move(rhs.mScratch))
    , mUnderscoreMemberName(std::move(rhs.mUnderscoreMemberName)) {}

ModuleBuilder::~ModuleBuilder() noexcept = default;

} // namespace Meta

} // namespace Cocos
