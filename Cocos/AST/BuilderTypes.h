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

enum Features : uint32_t {
    Fwd = 1 << 0,
    Types = 1 << 1,
    Serialization = 1 << 2,
    Names = 1 << 3,
    NVP = 1 << 4,
    Reflection = 1 << 5,
    Graphs = 1 << 6,
    Sender = 1 << 7,
    Typescripts = 1 << 8,
};

constexpr Features operator|(const Features lhs, const Features rhs) noexcept {
    return (Features)((uint32_t)lhs | (uint32_t)rhs);
}

constexpr Features operator&(const Features lhs, const Features rhs) noexcept {
    return (Features)((uint32_t)lhs & (uint32_t)rhs);
}

constexpr Features& operator|=(Features& lhs, const Features rhs) noexcept {
    return lhs = lhs | rhs;
}

constexpr Features& operator&=(Features& lhs, const Features rhs) noexcept {
    return lhs = lhs & rhs;
}

constexpr bool operator!(Features e) noexcept {
    return e == static_cast<Features>(0);
}

constexpr bool any(Features e) noexcept {
    return !!e;
}

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
    void addMember(SyntaxGraph::vertex_descriptor vertID, bool bPublic,
        std::string_view adlPath, std::string_view memberName,
        std::string_view initial = "_", GenerationFlags flags = {});
    void addConstructor(SyntaxGraph::vertex_descriptor vertID,
        std::initializer_list<std::string_view> members);
    void addTypescriptFunctions(SyntaxGraph::vertex_descriptor vertID, std::string_view content);

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
    bool mUnderscoreMemberName = false;
};

} // namespace Meta

} // namespace Cocos
