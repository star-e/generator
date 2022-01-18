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
#include <Cocos/AST/BuilderFwd.h>
#include <Cocos/AST/SyntaxTypes.h>

namespace Cocos {

namespace Meta {

struct TypeHandle {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mPathSuffix.get_allocator().resource());
    }

    TypeHandle(move_ptr<ModuleBuilder> moduleBuilder, const allocator_type& alloc);
    TypeHandle(move_ptr<ModuleBuilder> moduleBuilder, std::string_view pathSuffix, const allocator_type& alloc);
    TypeHandle(TypeHandle&& rhs, const allocator_type& alloc);

    TypeHandle(TypeHandle&& rhs) = default;
    TypeHandle(TypeHandle const& rhs) = delete;
    TypeHandle& operator=(TypeHandle&& rhs) = default;
    TypeHandle& operator=(TypeHandle const& rhs) = delete;
    ~TypeHandle() noexcept;

    move_ptr<ModuleBuilder> mModuleBuilder = nullptr;
    std::pmr::string mPathSuffix;
    uint32_t mVertexDescriptor = std::numeric_limits<uint32_t>::max();
};

struct ModuleHandle {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mModulePath.get_allocator().resource());
    }

    ModuleHandle(move_ptr<ModuleBuilder> moduleBuilder, const allocator_type& alloc);
    ModuleHandle(move_ptr<ModuleBuilder> moduleBuilder, std::string_view modulePath, const allocator_type& alloc);
    ModuleHandle(ModuleHandle&& rhs, const allocator_type& alloc);

    ModuleHandle(ModuleHandle&& rhs) = default;
    ModuleHandle(ModuleHandle const& rhs) = delete;
    ModuleHandle& operator=(ModuleHandle&& rhs) = default;
    ModuleHandle& operator=(ModuleHandle const& rhs) = delete;
    ~ModuleHandle() noexcept;

    move_ptr<ModuleBuilder> mModuleBuilder = nullptr;
    std::pmr::string mModulePath;
    uint32_t mVertexDescriptor = std::numeric_limits<uint32_t>::max();
};

struct CodegenScope {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mName.get_allocator().resource());
    }

    CodegenScope(std::string_view name, const allocator_type& alloc);
    CodegenScope(CodegenScope&& rhs, const allocator_type& alloc);
    CodegenScope(CodegenScope const& rhs, const allocator_type& alloc);

    CodegenScope(CodegenScope&& rhs) = default;
    CodegenScope(CodegenScope const& rhs) = delete;
    CodegenScope& operator=(CodegenScope&& rhs) = default;
    CodegenScope& operator=(CodegenScope const& rhs) = default;
    ~CodegenScope() noexcept;

    std::pmr::string mName;
    int32_t mCount = 0;
};

struct CodegenContext {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mScopes.get_allocator().resource());
    }

    CodegenContext(const allocator_type& alloc) noexcept;
    CodegenContext(CodegenContext&& rhs, const allocator_type& alloc);
    CodegenContext(CodegenContext const& rhs, const allocator_type& alloc);

    CodegenContext(CodegenContext&& rhs) = default;
    CodegenContext(CodegenContext const& rhs) = delete;
    CodegenContext& operator=(CodegenContext&& rhs) = default;
    CodegenContext& operator=(CodegenContext const& rhs) = default;
    ~CodegenContext() noexcept;

    std::pmr::list<CodegenScope> mScopes;
    std::pmr::vector<std::pmr::string> mNamespaces;
};

struct ModuleBuilder {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mSyntaxGraph.get_allocator().resource());
    }

    ModuleBuilder(const allocator_type& alloc);
    ModuleBuilder(ModuleBuilder&& rhs, const allocator_type& alloc);

    ModuleBuilder(ModuleBuilder&& rhs) = default;
    ModuleBuilder(ModuleBuilder const& rhs) = delete;
    ModuleBuilder& operator=(ModuleBuilder&& rhs) = default;
    ModuleBuilder& operator=(ModuleBuilder const& rhs) = delete;
    ~ModuleBuilder() noexcept;

    SyntaxGraph::vertex_descriptor registerType(std::string_view name, SyntaxGraph::vertex_tag_type tag);

    TypeHandle openNamespace(std::string_view namespaceSuffix);
    ModuleHandle openModule(std::string_view modulePath, ModuleInfo info = {});

    SyntaxGraph::vertex_descriptor addConcept(std::string_view name);
    SyntaxGraph::vertex_descriptor addContainer(std::string_view name, Traits traits);
    SyntaxGraph::vertex_descriptor addMap(std::string_view name, Traits traits);

    SyntaxGraph::vertex_descriptor addValue(std::string_view name);
    SyntaxGraph::vertex_descriptor addEnum(std::string_view name, Traits traits = {});
    SyntaxGraph::vertex_descriptor addFlags(std::string_view name, Traits traits = {});
    void addEnumElement(SyntaxGraph::vertex_descriptor vertID,
        std::string_view name, std::string_view value = {});
    SyntaxGraph::vertex_descriptor addTag(std::string_view name, bool bEntity = false,
        std::initializer_list<std::string_view> concepts = {});

    TypeHandle addStruct(std::string_view name, Traits traits = {});
    void addInherits(SyntaxGraph::vertex_descriptor vertID, std::string_view name);
    void addMember(SyntaxGraph::vertex_descriptor vertID, bool bPublic,
        std::string_view adlPath, std::string_view memberName,
        std::string_view initial = "_", GenerationFlags flags = {});
    void addConstructor(SyntaxGraph::vertex_descriptor vertID,
        std::initializer_list<std::string_view> members, bool hasDefault);
    void addTypescriptFunctions(SyntaxGraph::vertex_descriptor vertID, std::string_view content);

    void addConstraints(std::string_view conceptName, std::string_view typeName);
    void addConstraints(SyntaxGraph::vertex_descriptor vertID, std::string_view conceptName);

    SyntaxGraph::vertex_descriptor addVariant(std::string_view name, GenerationFlags flags = {});
    void addVariantElement(SyntaxGraph::vertex_descriptor vertID, std::string_view type);

    TypeHandle addGraph(std::string_view name, std::string_view vertex,
        std::string_view edge, Traits traits);

    void addGraphComponent(SyntaxGraph::vertex_descriptor vertID, std::string_view name,
        std::string_view type, std::string_view memberName);

    void addGraphPolymorphic(SyntaxGraph::vertex_descriptor vertID,
        std::string_view tag, std::string_view type, std::string_view memberName);

    void outputModule(const std::filesystem::path& rootFolder,
        std::string_view name, Features features) const;

    void projectTypescript(std::string_view cpp, std::string_view ts);

    // Generation
    std::pmr::string getMemberName(std::string_view memberName, bool bPublic) const;
    std::pmr::string getTypedMemberName(const Member& m, bool bPublic, bool bFull = false) const;

    SyntaxGraph mSyntaxGraph;
    ModuleGraph mModuleGraph;
    std::pmr::string mCurrentModule;
    std::pmr::string mCurrentScope;
    std::pmr::memory_resource* mScratch = std::pmr::get_default_resource();
    bool mUnderscoreMemberName = true;
};

} // namespace Meta

} // namespace Cocos
