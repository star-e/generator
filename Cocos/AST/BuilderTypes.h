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
#include <Cocos/AST/BuilderFwd.h>
#include <Cocos/AST/SyntaxTypes.h>

namespace Cocos {

namespace Meta {

enum class BreakType : uint32_t {
    Default,
    Alias,
    Enum,
    Tag,
    Variant,
    Struct,
};

enum class OperatorType : uint32_t {
    CopyAssign,
    MoveAssign,
    Equal,
    Unequal,
    Less,
    Spaceship,
    Hash,
};

struct CppStructBuilder {
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mCurrentNamespace.get_allocator().resource());
    }

    CppStructBuilder(const allocator_type& alloc);
    CppStructBuilder(const SyntaxGraph* syntaxGraph, const ModuleGraph* moduleGraph, uint32_t currentVertex, uint32_t currentModule, std::string_view currentNamespace, std::string_view projectName, const allocator_type& alloc);
    CppStructBuilder(CppStructBuilder&& rhs, const allocator_type& alloc);
    CppStructBuilder(CppStructBuilder const& rhs, const allocator_type& alloc);

    CppStructBuilder(CppStructBuilder&& rhs) = default;
    CppStructBuilder(CppStructBuilder const& rhs) = delete;
    CppStructBuilder& operator=(CppStructBuilder&& rhs) = default;
    CppStructBuilder& operator=(CppStructBuilder const& rhs) = default;
    ~CppStructBuilder() noexcept;

    const Traits& getTraits() const;
    std::pmr::string getDependentName(SyntaxGraph::vertex_descriptor vertID) const;
    std::pmr::string getDependentName(std::string_view typePath) const;
    std::pmr::string getImplName(std::string_view ns = ".") const;
    std::pmr::string generateGetAllocatorSignature(bool bInline) const;
    std::pmr::string generateGetAllocatorBody() const;
    std::pmr::string generateMembers() const;
    std::pmr::string generateOperatorSignature(OperatorType type, bool bInline = false) const;
    std::pmr::string generateOperatorBody(OperatorType type) const;
    std::pmr::string generateHeaderConstructors() const;
    std::pmr::string generateCppConstructors() const;
    std::pmr::string generateConstructorSignature(const Constructor& cntr, bool bInline) const;
    std::pmr::string generateConstructorBody(const Constructor& cntr) const;
    std::pmr::string generateConstructorCall(SyntaxGraph::vertex_descriptor vertID, const Constructor& cntr) const;
    std::pmr::string generateMemberFunctions(std::pmr::string& space) const;
    std::pmr::string generateMethod(const Method& method, bool bOverride, bool bDefaultParam = false) const;
    std::pmr::string generateDispatchMethods(const Method& method) const;

    const SyntaxGraph* mSyntaxGraph = nullptr;
    const ModuleGraph* mModuleGraph = nullptr;
    uint32_t mCurrentVertex = SyntaxGraph::null_vertex();
    uint32_t mCurrentModule = ModuleGraph::null_vertex();
    std::pmr::string mCurrentNamespace;
    std::pmr::string mCurrentPath;
    std::pmr::string mName;
    std::pmr::string mAPI;
    std::pmr::string mProjectName;
    bool mBoost = true;
};

class GraphBuilder {
public:
    GraphBuilder() noexcept = default;
    GraphBuilder(GraphBuilder&& rhs) = delete;
    GraphBuilder(GraphBuilder const& rhs) = delete;
    GraphBuilder& operator=(GraphBuilder&& rhs) = delete;
    GraphBuilder& operator=(GraphBuilder const& rhs) = delete;
    virtual ~GraphBuilder() noexcept = 0;

    virtual std::pmr::string nullVertexValue() const = 0;
    virtual std::pmr::string vertexDescType() const = 0;
    virtual std::pmr::string edgeDescType() const = 0;

    virtual std::pmr::string graphType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string vertexDesc(std::string_view ns = ".") const = 0;
    virtual std::pmr::string edgeDesc(std::string_view ns = ".") const = 0;
    virtual std::pmr::string vertexType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string edgeType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string vertexPropertyType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string edgePropertyType(std::string_view ns = ".") const = 0;

    virtual std::pmr::string objectListType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string vertexListType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string componentContainerType() const = 0;
    virtual std::pmr::string edgeListType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string outEdgeListType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string outEdgeType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string outIterType() const = 0;
    virtual std::pmr::string inEdgeListType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string inEdgeType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string inIterType() const = 0;
    virtual std::pmr::string edgeIterType() const = 0;
    virtual std::pmr::string childListType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string childEdgeType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string childIterType() const = 0;
    virtual std::pmr::string parentListType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string parentEdgeType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string parentIterType() const = 0;
    virtual std::pmr::string referenceIterType() const = 0;
    virtual std::pmr::string vertexIterType() const = 0;
    virtual std::pmr::string adjIterType() const = 0;

    virtual std::pmr::string tagType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string valueType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string constValueType(std::string_view ns = ".") const = 0;
    virtual std::pmr::string handleElemType(const PolymorphicPair& pair,
        std::string_view ns = ".", bool bSkipName = false) const = 0;
    virtual std::pmr::string handleType(std::string_view ns = ".") const = 0;

    virtual std::pmr::string vertexPropertyMapName(bool bConst) const = 0;
    virtual std::pmr::string vertexPropertyMapMemberName(bool bConst) const = 0;
    virtual std::pmr::string edgePropertyMapName(bool bConst) const = 0;
    virtual std::pmr::string edgePropertyMapMemberName(bool bConst, std::string_view stringPath = "") const = 0;
    virtual std::pmr::string vertexComponentMapName(const Component& c, bool bConst) const = 0;
    virtual std::pmr::string vertexComponentMapMemberName(const Component& c,
        bool bConst, std::string_view stringPath = "") const = 0;

    virtual std::pmr::string generateVertexType(std::string_view name, bool layer) const = 0;

    virtual std::pmr::string generateAllocator_h() = 0;
    virtual std::pmr::string generateConstructors_h() = 0;
    virtual std::pmr::string generateGraph_h() = 0;
    virtual std::pmr::string generateIncidenceGraph_h() const = 0;
    virtual std::pmr::string generateBidirectionalGraph_h() const = 0;
    virtual std::pmr::string generateAdjacencyGraph_h() const = 0;
    virtual std::pmr::string generateVertexListGraph_h() const = 0;
    virtual std::pmr::string generateEdgeListGraph_h() const = 0;
    virtual std::pmr::string generateMutableGraph_h() const = 0;
    virtual std::pmr::string generateReferenceGraph_h() const = 0;
    virtual std::pmr::string generateParentGraph_h() const = 0;
    virtual std::pmr::string generateAddressableGraph_h() const = 0;
    virtual std::pmr::string generatePolymorphicGraph_h() const = 0;
    virtual std::pmr::string generateMemberFunctions_h() const = 0;
    virtual std::pmr::string generateReserve_h() const = 0;
    virtual std::pmr::string generateTags_h() const = 0;
    virtual std::pmr::string generateMembers_h() const = 0;

    virtual std::pmr::string generateReserve_cpp() const = 0;

    virtual std::pmr::string generateAddEdge(bool property,
        bool cntr, bool variadic, bool reference, bool function = true) const = 0;
    virtual std::pmr::string generateRemoveEdges(bool bInline = true) const = 0;
    virtual std::pmr::string generateRemoveEdge(bool bInline = true) const = 0;
    virtual std::pmr::string generateRemoveEdgeIter(bool bInline = true) const = 0;
    virtual std::pmr::string addVertex(bool propertyParam, bool piecewise, bool cpp14 = false) const = 0;
    virtual std::pmr::string generateAddressableGraph(bool bInline = true) const = 0;

    virtual std::pmr::string generateGraphFunctions_h() const = 0;
    virtual std::pmr::string generateGraphBoostFunctions_h() const = 0;
    virtual std::pmr::string generateGraphPropertyMaps_h() const = 0;
    virtual std::pmr::string generateGraphSerialization_h(bool nvp) const = 0;
};

class CppGraphBuilder final : public GraphBuilder {
public:
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
    allocator_type get_allocator() const noexcept {
        return allocator_type(mStruct.get_allocator().resource());
    }

    CppGraphBuilder(const allocator_type& alloc) noexcept;
    ~CppGraphBuilder() noexcept override;

    std::pmr::string nullVertexValue() const override;
    std::pmr::string vertexDescType() const override;
    std::pmr::string edgeDescType() const override;

    std::pmr::string graphType(std::string_view ns = ".") const override;
    std::pmr::string vertexDesc(std::string_view ns = ".") const override;
    std::pmr::string edgeDesc(std::string_view ns = ".") const override;
    std::pmr::string vertexType(std::string_view ns = ".") const override;
    std::pmr::string edgeType(std::string_view ns = ".") const override;
    std::pmr::string vertexPropertyType(std::string_view ns = ".") const override;
    std::pmr::string edgePropertyType(std::string_view ns = ".") const override;

    std::pmr::string objectListType(std::string_view ns = ".") const override;
    std::pmr::string vertexListType(std::string_view ns = ".") const override;
    std::pmr::string componentContainerType() const override;
    std::pmr::string edgeListType(std::string_view ns = ".") const override;
    std::pmr::string outEdgeListType(std::string_view ns = ".") const override;
    std::pmr::string outEdgeType(std::string_view ns = ".") const override;
    std::pmr::string outIterType() const override;
    std::pmr::string inEdgeListType(std::string_view ns = ".") const override;
    std::pmr::string inEdgeType(std::string_view ns = ".") const override;
    std::pmr::string inIterType() const override;
    std::pmr::string edgeIterType() const override;
    std::pmr::string childListType(std::string_view ns = ".") const override;
    std::pmr::string childEdgeType(std::string_view ns = ".") const override;
    std::pmr::string childIterType() const override;
    std::pmr::string parentListType(std::string_view ns = ".") const override;
    std::pmr::string parentEdgeType(std::string_view ns = ".") const override;
    std::pmr::string parentIterType() const override;
    std::pmr::string referenceIterType() const override;
    std::pmr::string vertexIterType() const override;
    std::pmr::string adjIterType() const override;

    std::pmr::string tagType(std::string_view ns = ".") const override;
    std::pmr::string valueType(std::string_view ns = ".") const override;
    std::pmr::string constValueType(std::string_view ns = ".") const override;
    std::pmr::string handleElemType(const PolymorphicPair& pair,
        std::string_view ns = ".", bool bSkipName = false) const override;
    std::pmr::string handleType(std::string_view ns = ".") const override;

    std::pmr::string vertexPropertyMapName(bool bConst) const override;
    std::pmr::string vertexPropertyMapMemberName(bool bConst) const override;
    std::pmr::string edgePropertyMapName(bool bConst) const override;
    std::pmr::string edgePropertyMapMemberName(bool bConst, std::string_view stringPath = "") const override;
    std::pmr::string vertexComponentMapName(const Component& c, bool bConst) const override;
    std::pmr::string vertexComponentMapMemberName(const Component& c,
        bool bConst, std::string_view stringPath = "") const override;

    std::pmr::string generateVertexType(std::string_view name, bool layer) const override;

    std::pmr::string generateAllocator_h() override;
    std::pmr::string generateConstructors_h() override;
    std::pmr::string generateGraph_h() override;
    std::pmr::string generateIncidenceGraph_h() const override;
    std::pmr::string generateBidirectionalGraph_h() const override;
    std::pmr::string generateAdjacencyGraph_h() const override;
    std::pmr::string generateVertexListGraph_h() const override;
    std::pmr::string generateEdgeListGraph_h() const override;
    std::pmr::string generateMutableGraph_h() const override;
    std::pmr::string generateReferenceGraph_h() const override;
    std::pmr::string generateParentGraph_h() const override;
    std::pmr::string generateAddressableGraph_h() const override;
    std::pmr::string generatePolymorphicGraph_h() const override;
    std::pmr::string generateMemberFunctions_h() const override;
    std::pmr::string generateReserve_h() const override;
    std::pmr::string generateTags_h() const override;
    std::pmr::string generateMembers_h() const override;

    std::pmr::string generateReserve_cpp() const override;

    std::pmr::string generateAddEdge(bool property,
        bool cntr, bool variadic, bool reference, bool function = true) const override;
    std::pmr::string generateRemoveEdges(bool bInline = true) const override;
    std::pmr::string generateRemoveEdge(bool bInline = true) const override;
    std::pmr::string generateRemoveEdgeIter(bool bInline = true) const override;
    std::pmr::string addVertex(bool propertyParam, bool piecewise, bool cpp14 = false) const override;
    std::pmr::string generateAddressableGraph(bool bInline = true) const override;

    std::pmr::string generateGraphFunctions_h() const override;
    std::pmr::string generateGraphBoostFunctions_h() const override;
    std::pmr::string generateGraphPropertyMaps_h() const override;
    std::pmr::string generateGraphSerialization_h(bool nvp) const override;

    CppGraphBuilder(const SyntaxGraph* syntaxGraph, const ModuleGraph* moduleGraph,
        uint32_t currentVertex, uint32_t currentModule,
        std::string_view currentNamespace,
        bool bDLL, std::string_view projectName, const allocator_type& alloc);

    void prepareNamespace(std::string_view& ns) const;

    CppStructBuilder mStruct;
    const Graph* mGraph = nullptr;
    bool mDLL = false;
    uint32_t mCount = 0;
    std::pmr::string mVertexType;
};

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

    uint32_t mVertexDescriptor = std::numeric_limits<uint32_t>::max();
    move_ptr<ModuleBuilder> mModuleBuilder = nullptr;
    std::pmr::string mPathSuffix;
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

    ModuleBuilder(std::string_view projectName, std::filesystem::path cppFolder, std::filesystem::path typescriptFolder, std::pmr::memory_resource* scratch, const allocator_type& alloc);
    ModuleBuilder(ModuleBuilder&& rhs, const allocator_type& alloc);

    ModuleBuilder(ModuleBuilder&& rhs) = default;
    ModuleBuilder(ModuleBuilder const& rhs) = delete;
    ModuleBuilder& operator=(ModuleBuilder&& rhs) = default;
    ModuleBuilder& operator=(ModuleBuilder const& rhs) = delete;
    ~ModuleBuilder() noexcept;
    void init();
    const SyntaxGraph& syntax() const noexcept {
        return mSyntaxGraph;
    }
    SyntaxGraph& syntax() noexcept {
        return mSyntaxGraph;
    }
    SyntaxGraph::vertex_descriptor null_vertex() const noexcept {
        return mSyntaxGraph.null_vertex();
    }
    SyntaxGraph::vertex_descriptor registerType(std::string_view name, SyntaxGraph::vertex_tag_type tag);

    TypeHandle openNamespace(std::string_view namespaceSuffix);
    ModuleHandle openModule(std::string_view modulePath, ModuleInfo info = {});

    SyntaxGraph::vertex_descriptor addDefine(std::string_view name, std::string_view content = {});
    SyntaxGraph::vertex_descriptor addConcept(std::string_view name, std::string_view parent = {});
    SyntaxGraph::vertex_descriptor addAlias(std::string_view name, std::string_view type);
    SyntaxGraph::vertex_descriptor addContainer(std::string_view name, Traits traits);
    SyntaxGraph::vertex_descriptor addMap(std::string_view name, Traits traits);

    SyntaxGraph::vertex_descriptor addValue(std::string_view name);
    SyntaxGraph::vertex_descriptor addEnum(std::string_view name, Traits traits = {});
    SyntaxGraph::vertex_descriptor addFlag(std::string_view name, Traits traits = {});
    void addEnumElement(SyntaxGraph::vertex_descriptor vertID,
        std::string_view name, std::string_view value);
    void setEnumUnderlyingType(SyntaxGraph::vertex_descriptor vertID, std::string_view type);
    SyntaxGraph::vertex_descriptor addTag(std::string_view name, bool bEntity = true,
        std::initializer_list<std::string_view> concepts = {});

    TypeHandle addStruct(std::string_view name, Traits traits = {});

    void addInherits(SyntaxGraph::vertex_descriptor vertID, std::string_view name);

    Member& addMember(SyntaxGraph::vertex_descriptor vertID, bool bPublic,
        std::string_view adlPath, std::string_view memberName,
        std::string_view initial = "_", GenerationFlags flags = {},
        std::string_view comments = {});

    void setMemberFlags(SyntaxGraph::vertex_descriptor vertID,
        std::string_view memberName, GenerationFlags flags, bool bOptional = false);

    void setTypescriptInitValue(SyntaxGraph::vertex_descriptor vertID,
        std::string_view memberName, std::string_view init);

    void addConstructor(SyntaxGraph::vertex_descriptor vertID,
        std::initializer_list<std::string_view> members, bool hasDefault);
    void addMemberFunctions(SyntaxGraph::vertex_descriptor vertID, std::string_view content);
    void addMethods(SyntaxGraph::vertex_descriptor vertID, std::string_view content);

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

    void addVertexMap(SyntaxGraph::vertex_descriptor vertID,
        std::string_view mapType, std::string_view memberName,
        std::string_view keyType);

    void addVertexBimap(SyntaxGraph::vertex_descriptor vertID,
        std::string_view mapType, std::string_view memberName,
        std::string_view componentName, std::string_view componentMemberName = "");

    void addNamedConcept(SyntaxGraph::vertex_descriptor vertID, bool bComponent,
        std::string_view componentName, std::string_view componentMemberName = "");

    void outputModule(std::string_view name, std::pmr::set<std::pmr::string>& files) const;

    void projectTypescript(std::string_view cpp, std::string_view ts);
    void addTypescriptFunctions(SyntaxGraph::vertex_descriptor vertID, std::string_view content);

    // Generation
    int compile();
    std::pmr::string getTypedMemberName(const Member& m, bool bPublic, bool bFull = false) const;
    std::pmr::string getTypedParameterName(const Parameter& m, bool bPublic, bool bFull = false, bool bOptional = false) const;

    std::pmr::string getTypescriptVertexName(SyntaxGraph::vertex_descriptor vertID,
        std::string_view descName) const;

    std::filesystem::path mCppFolder;
    std::filesystem::path mTypescriptFolder;
    SyntaxGraph mSyntaxGraph;
    ModuleGraph mModuleGraph;
    std::pmr::string mCurrentModule;
    std::pmr::string mCurrentScope;
    std::pmr::string mProjectName;
    std::pmr::memory_resource* mScratch = nullptr;
    bool mUnderscoreMemberName = true;
    bool mCompiled = false;
    bool mBoost = true;
};

} // namespace Meta

} // namespace Cocos
