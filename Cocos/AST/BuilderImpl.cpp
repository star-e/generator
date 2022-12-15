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

#include "BuilderTypes.h"
#include "SyntaxGraphs.h"
#include <Cocos/Path.h>
#include "SyntaxUtils.h"
#include "TypescriptBuilder.h"
#include "BuilderMacros.h"
#include "CppBuilder.h"
#include "JsbBuilder.h"
#include "ToJsBuilder.h"
#include "CppMethod.h"
#include "SwigConfig.h"

namespace Cocos::Meta {

TypeHandle::TypeHandle(move_ptr<ModuleBuilder> moduleBuilder,
    const allocator_type& alloc)
    : mModuleBuilder(std::move(moduleBuilder))
    , mPathSuffix(alloc) {
}

TypeHandle::TypeHandle(move_ptr<ModuleBuilder> moduleBuilder,
    std::string_view suffix, const allocator_type& alloc)
    : mModuleBuilder(std::move(moduleBuilder))
    , mPathSuffix(suffix, alloc) {
    
}

TypeHandle::~TypeHandle() {
    if (!mModuleBuilder)
        return;

    Expects(!mPathSuffix.empty());
    // length(name) + 1<= length(current scope)
    Expects(mPathSuffix.size() + 1 <= mModuleBuilder->mCurrentScope.size());

    auto& currentScope = mModuleBuilder->mCurrentScope;
    Expects(isTypePath(currentScope));

    { // last namespace should equal
        auto iter1 = currentScope.rbegin();
        for (auto iter = mPathSuffix.rbegin(); iter != mPathSuffix.rend(); ++iter, ++iter1) {
            Expects(*iter1 == *iter);
        }
    }
    auto range = boost::algorithm::find_last(currentScope, mPathSuffix);
    Expects(!range.empty());
    if (range.begin() != currentScope.begin() + 1) {
        auto count = std::distance(currentScope.begin(), range.begin());
        Expects(count > 0);
        currentScope = currentScope.substr(0, count - 1);
    } else {
        Expects(mPathSuffix.size() + 1 == currentScope.size());
        currentScope.clear();
    }
}

ModuleHandle::ModuleHandle(move_ptr<ModuleBuilder> moduleBuilder,
    const allocator_type& alloc)
    : mModuleBuilder(std::move(moduleBuilder))
    , mModulePath(alloc) {
}

ModuleHandle::ModuleHandle(move_ptr<ModuleBuilder> moduleBuilder,
    std::string_view suffix, const allocator_type& alloc)
    : mModuleBuilder(std::move(moduleBuilder))
    , mModulePath(suffix, alloc) {
}

ModuleHandle::~ModuleHandle() {
    if (!mModuleBuilder)
        return;
    std::string_view path(mModuleBuilder->mCurrentModule);
    Expects(!path.empty());
    Expects(mModulePath == path.substr(1));
    mModuleBuilder->mCurrentModule.clear();
}

void ModuleBuilder::init() {
    mSyntaxGraph.mScratch = mScratch;
}

ModuleGraph::vertex_descriptor ModuleBuilder::registerType(
    std::string_view name, SyntaxGraph::vertex_tag_type tag) {
    auto& g = mSyntaxGraph;
    auto parentID = locate(mCurrentScope, g);
    if (!mCurrentScope.empty()) {
        Expects(parentID != g.null_vertex());
    }

    return std::visit(
        overload(
            [&](auto t) {
                return add_vertex(t,
                    std::forward_as_tuple(name), // name
                    std::forward_as_tuple(), // trait
                    std::forward_as_tuple(), // requires
                    std::forward_as_tuple(), // inherits
                    std::forward_as_tuple(mCurrentModule), // module path
                    std::forward_as_tuple(), // typescript
                    std::forward_as_tuple(), // polymorphic
                    g, parentID);
            }
        ), tag);
}

namespace {

void appendPath(std::pmr::string& currentPath, std::string_view name) {
    std::pmr::string validPath(name, currentPath.get_allocator());
    convertTypename(validPath);

    currentPath.append("/");
    currentPath.append(validPath);
}

TypeHandle appendTypePath(ModuleBuilder& builder, std::string_view name,
    SyntaxGraph::vertex_descriptor vertID = SyntaxGraph::null_vertex()) {
    TypeHandle wrapper(&builder, builder.get_allocator());

    wrapper.mPathSuffix = name;
    wrapper.mVertexDescriptor = vertID;

    appendPath(builder.mCurrentScope, name);

    return wrapper;
}

ModuleHandle appendModulePath(ModuleBuilder& builder, std::string_view name,
    SyntaxGraph::vertex_descriptor vertID = SyntaxGraph::null_vertex()) {
    ModuleHandle wrapper(&builder, builder.get_allocator());

    wrapper.mModulePath = name;
    wrapper.mVertexDescriptor = vertID;

    appendPath(builder.mCurrentModule, name);

    return wrapper;
}

}

TypeHandle ModuleBuilder::openNamespace(std::string_view name) {
    auto& g = mSyntaxGraph;
    auto parentID = locate(mCurrentScope, g);
    Expects(parentID == SyntaxGraph::null_vertex()
        || holds_tag<Namespace_>(parentID, g));

    auto vertID = locate(parentID, name, g);
    if (vertID == g.null_vertex()) {
        vertID = add_vertex(Namespace_{},
            std::forward_as_tuple(name), // name
            std::forward_as_tuple(), // trait
            std::forward_as_tuple(), // requires
            std::forward_as_tuple(), // inherits
            std::forward_as_tuple(), // module path
            std::forward_as_tuple(), // typescript
            std::forward_as_tuple(), // polymorphic
            g, parentID);
    }

    return appendTypePath(*this, name);
}

ModuleHandle ModuleBuilder::openModule(std::string_view name, ModuleInfo info) {
    auto parentID = locate(mCurrentModule, mModuleGraph);
    if (!mCurrentModule.empty()) {
        Expects(parentID != mModuleGraph.null_vertex());
    }

    auto vertID = locate(parentID, name, mModuleGraph);
    if (vertID == mModuleGraph.null_vertex()) {
        vertID = add_vertex(std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(std::move(info)),
            mModuleGraph, parentID);
    }

    return appendModulePath(*this, name, vertID);
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addDefine(std::string_view name, std::string_view content) {
    auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    Expects(locate(g.null_vertex(), name, g) == g.null_vertex());

    auto vertID = add_vertex(Define_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(content), // polymorphic
        g, g.null_vertex());

    return vertID;
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addConcept(std::string_view name, std::string_view parent) {
    if (parent == "_")
        parent = "";

    auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    std::pmr::string superTypePath(scratch);
    if (!parent.empty()) {
        auto superType = convertTypename(parent, scratch);
        auto superID = g.lookupType(mCurrentScope, superType, scratch);
        Expects(superID != g.null_vertex());
        superTypePath = g.getTypePath(superID, scratch);
    }

    auto vertID = add_vertex(Concept_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(superTypePath), // polymorphic
        g, parentID);

    return vertID;
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addAlias(std::string_view name, std::string_view type) {
    auto& g = mSyntaxGraph;
    auto scratch = get_allocator().resource();

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    std::pmr::string typePath(g.get_allocator().resource());
    if (!type.empty()) {
        std::pmr::string adlPath(type, scratch);
        convertTypename(adlPath);

        if (isInstance(adlPath)) {
            g.instantiate(mCurrentScope, adlPath, scratch);
        }
        auto typeID = g.lookupType(mCurrentScope, adlPath, scratch);
        typePath = g.getTypePath(typeID, g.get_allocator().resource());
    }

    auto vertID = add_vertex(Alias_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(std::move(typePath)), // polymorphic
        g, parentID);

    return vertID;
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addContainer(std::string_view name, Traits traits) {
    auto& g = mSyntaxGraph;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Container_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits)), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    return vertID;
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addMap(std::string_view name, Traits traits) {
    auto& g = mSyntaxGraph;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Map_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits)), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    return vertID;
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addValue(std::string_view name) {
    auto& g = mSyntaxGraph;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Value_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    return vertID;
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addEnum(std::string_view name, Traits traits) {
    auto& g = mSyntaxGraph;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Enum_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits)), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    auto& e = get<Enum>(vertID, g);

    return vertID;
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addFlag(std::string_view name, Traits traits) {
    bool bEnumOperator = !(traits.mFlags & NO_ENUM_OPERATOR);

    auto& g = mSyntaxGraph;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Enum_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits)), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    auto& e = get<Enum>(vertID, g);
    e.mIsFlags = true;
    e.mEnumOperator = bEnumOperator;
    e.mUnderlyingType = "uint32_t";

    return vertID;
}

void ModuleBuilder::addEnumElement(SyntaxGraph::vertex_descriptor vertID,
    std::string_view name, std::string_view value) {
    auto& g = mSyntaxGraph;
    Expects(holds_tag<Enum_>(vertID, g));

    if (value == "_")
        value = {};

    auto& e = get_by_tag<Enum_>(vertID, g);
    EnumValue v(e.get_allocator());
    v.mName = name;
    v.mValue = value;
    e.mValues.emplace_back(std::move(v));
}

void ModuleBuilder::setEnumUnderlyingType(SyntaxGraph::vertex_descriptor vertID,
    std::string_view type) {
    auto& g = mSyntaxGraph;
    Expects(holds_tag<Enum_>(vertID, g));

    auto& e = get_by_tag<Enum_>(vertID, g);
    e.mUnderlyingType = type;
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addTag(std::string_view name, bool bEntity,
    std::initializer_list<std::string_view> concepts) {
    auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Tag_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(Tag{ .mEntity = bEntity }), // polymorphic
        g, parentID);

    auto& constraints = get(g.constraints, g, vertID);
    for (const auto& c : concepts) {
        if (c == "_")
            continue;

        auto conceptName = convertTypename(c, scratch);
        auto conceptID = g.lookupIdentifier(mCurrentScope, conceptName, scratch);
        Expects(conceptID != g.null_vertex());
        auto conceptPath = g.getTypePath(conceptID, g.get_allocator().resource());
        constraints.mConcepts.emplace_back(conceptPath);
    }

    return vertID;
}

TypeHandle ModuleBuilder::addStruct(std::string_view name, Traits traits) {
    auto& g = mSyntaxGraph;

    if (traits.mInterface) {
        traits.mFlags |= NO_MOVE_NO_COPY;
    }

    auto parentID = locate(mCurrentScope, g);
    Expects(parentID == SyntaxGraph::null_vertex()
        || holds_tag<Namespace_>(parentID, g)
        || holds_tag<Struct_>(parentID, g)
        || holds_tag<Graph_>(parentID, g));

    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Struct_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits)), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    return appendTypePath(*this, name, vertID);
}

void ModuleBuilder::addInherits(SyntaxGraph::vertex_descriptor vertID, std::string_view type) {
    auto& g = mSyntaxGraph;
    auto scratch = get_allocator().resource();

    std::pmr::string adlPath(type, scratch);
    convertTypename(adlPath);
    Expects(!isInstance(adlPath));
    auto baseID = g.lookupType(mCurrentScope, adlPath, scratch);
    Expects(baseID != g.null_vertex());
    const auto& baseTraits = get(g.traits, g, baseID);
    Expects(baseTraits.mInterface);
    auto& inherits = get(g.inherits, g, vertID);

    auto typePath = g.getTypePath(baseID, g.get_allocator().resource());
    inherits.mBases.emplace_back(std::move(typePath));
    auto& traits = get(g.traits, g, vertID);
    traits.mClass = true;
}

Member& ModuleBuilder::addMember(SyntaxGraph::vertex_descriptor vertID, bool bPublic,
    std::string_view className, std::string_view memberName,
    std::string_view initial, GenerationFlags flags,
    std::string_view comments) {
    auto& g = mSyntaxGraph;
    auto scratch = get_allocator().resource();

    std::pmr::string adlPath(className, scratch);

    bool bOptional = false;
    if (boost::algorithm::contains(adlPath, "[[optional]]")) {
        bOptional = true;
        boost::algorithm::replace_all(adlPath, "[[optional]]", "");
        boost::algorithm::trim(adlPath);
    }

    convertTypename(adlPath);

    memberName = boost::algorithm::trim_copy(memberName);
    if (initial == "_") {
        initial = {};
    }

    bool bInstance = isInstance(adlPath);
    if (bInstance) {
        g.instantiate(mCurrentScope, adlPath, scratch);
    }

    Member* ptr = nullptr;

    auto addMember = [&](auto& s) {
        std::pmr::string typeName(adlPath, scratch);
        Member m(get_allocator());

        if (!bInstance) {
            auto astPos = adlPath.find('*');
            if (astPos != adlPath.npos) {
                m.mPointer = true;
                if (auto pos = adlPath.find('&', astPos); pos != adlPath.npos) {
                    m.mReference = true;
                }
                typeName.resize(astPos);
            } else {
                if (auto pos = adlPath.find('&'); pos != adlPath.npos) {
                    m.mReference = true;
                    typeName.resize(pos);
                }
            }
            if (boost::algorithm::contains(typeName, "const ") || boost::algorithm::contains(typeName, " const")) {
                m.mConst = true;
                boost::algorithm::replace_first(typeName, "const ", "");
                boost::algorithm::replace_first(typeName, " const", "");
                boost::algorithm::trim(typeName);
            }
        }

        auto vertID = g.lookupType(mCurrentScope, typeName, scratch);
        if (vertID == g.null_vertex()) {
            Expects(m.mPointer || m.mReference);
            m.mTypePath = typeName;
        } else {
            auto typePath = g.getTypePath(vertID, scratch);
            m.mTypePath = std::move(typePath);
        }

        m.mPublic = bPublic;
        m.mMemberName = memberName;
        m.mDefaultValue = initial;
        if (!m.mDefaultValue.empty() && m.mDefaultValue.front() == '('
            && m.mDefaultValue.back() == ')') {
            m.mDefaultValue.pop_back();
            m.mDefaultValue.erase(m.mDefaultValue.begin());
        }
        m.mFlags = flags;
        m.mComments = comments;
        m.mTypescriptOptional = bOptional;

        s.mMembers.emplace_back(std::move(m));
        ptr = &s.mMembers.back();
    };

    visit_vertex(
        vertID, g,
        [&](Composition_ auto& s) {
            addMember(s);
        },
        [&](const auto&) {
            Expects(false);
        });

    Expects(ptr);
    return *ptr;
}

void ModuleBuilder::setMemberFlags(SyntaxGraph::vertex_descriptor vertID,
    std::string_view memberName, GenerationFlags flags, bool bOptional) {
    auto& g = mSyntaxGraph;
    visit_vertex(
        vertID, g,
        [&](Composition_ auto& s) {
            for (Member& m : s.mMembers) {
                if (m.mMemberName == memberName) {
                    m.mFlags = flags;
                    m.mTypescriptOptional = bOptional;
                    break;
                }
            }
        },
        [&](const auto&) {
        });
}

void ModuleBuilder::setTypescriptInitValue(SyntaxGraph::vertex_descriptor vertID,
    std::string_view memberName, std::string_view init) {
    auto& g = mSyntaxGraph;
    visit_vertex(
        vertID, g,
        [&](Composition_ auto& s) {
            for (Member& m : s.mMembers) {
                if (m.mMemberName == memberName) {
                    m.mTypescriptDefaultValue = init;
                    break;
                }
            }
        },
        [&](const auto&) {
        });
}

void ModuleBuilder::addConstructor(SyntaxGraph::vertex_descriptor vertID,
    std::initializer_list<std::string_view> members, bool hasDefault) {
    auto& g = mSyntaxGraph;
    visit_vertex(
        vertID, g,
        [&](Composition_ auto& s) {
            auto& cntr = s.mConstructors.emplace_back();
            cntr.mHasDefault = hasDefault;
            for (const auto& memberName : members) {
                uint32_t id = 0;
                for (const auto& m : s.mMembers) {
                    if (memberName == m.mMemberName) {
                        break;
                    }
                    ++id;
                }
                if (id == s.mMembers.size()) {
                    throw std::invalid_argument("cntr: member not found");
                }
                cntr.mIndices.emplace_back(id);
            }
        },
        [&](const auto&) {
        });
}

void ModuleBuilder::addMemberFunctions(SyntaxGraph::vertex_descriptor vertID,
    std::string_view content) {
    auto& g = mSyntaxGraph;
    visit_vertex(
        vertID, g,
        [&](Composition_ auto& s) {
            s.mMemberFunctions.emplace_back(content);
        },
        [&](const auto&) {
        });
}

void ModuleBuilder::addMethods(SyntaxGraph::vertex_descriptor vertID,
    std::string_view content) {
    auto& g = mSyntaxGraph;
    visit_vertex(
        vertID, g,
        [&](Composition_ auto& s) {
            s.mMethods = parseFunctions(*this, content);
        },
        [&](const auto&) {
        });
}

void ModuleBuilder::addConstraints(std::string_view conceptName0,
    std::string_view typeName0) {
    auto scratch = mScratch;
    auto& g = mSyntaxGraph;

    auto typeName = convertTypename(typeName0, scratch);
    auto typeID = g.lookupIdentifier(mCurrentScope, typeName, scratch);
    Expects(typeID != g.null_vertex());

    addConstraints(typeID, conceptName0);
}

void ModuleBuilder::addConstraints(SyntaxGraph::vertex_descriptor typeID, std::string_view conceptName0) {
    auto& g = mSyntaxGraph;
    const auto& mg = mModuleGraph;
    auto scratch = mScratch;
    auto conceptName = convertTypename(conceptName0, scratch);
    auto conceptID = g.lookupIdentifier(mCurrentScope, conceptName, scratch);
    Expects(conceptID != g.null_vertex());

    auto typeModuleID = locate(get(g.modulePaths, g, typeID), mg);
    auto superModuleID = locate(get(g.modulePaths, g, conceptID), mg);
    if (superModuleID != typeModuleID && !edge(typeModuleID, superModuleID, mg).second) {
        Expects(false);
        throw std::out_of_range("supertype not in dependencies");
    }

    auto& constraints = get(g.constraints, g, typeID);
    auto supertypePath = g.getTypePath(conceptID, mScratch);
    constraints.mConcepts.emplace_back(supertypePath);
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addVariant(
    std::string_view name, GenerationFlags flags) {
    auto& g = mSyntaxGraph;

    Expects(g.isNamespace(mCurrentScope));
    auto parentID = locate(mCurrentScope, g);

    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Variant_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(Traits{ .mFlags = flags }), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    return vertID;
}

void ModuleBuilder::addVariantElement(SyntaxGraph::vertex_descriptor vertID,
    std::string_view type) {
    auto& g = mSyntaxGraph;
    auto scratch = mScratch;
    Expects(holds_tag<Variant_>(vertID, g));
    auto& var = get<Variant>(vertID, g);

    std::pmr::string typePath(type, scratch);
    convertTypename(typePath);

    var.mVariants.emplace_back(g.getTypePath(mCurrentScope, typePath,
        g.get_allocator().resource(), mScratch));
}

TypeHandle ModuleBuilder::addGraph(std::string_view name,
    std::string_view vertex, std::string_view edge, Traits traits0) {
    if (traits0.mInterface) {
        traits0.mFlags |= NO_MOVE_NO_COPY;
    }

    auto& g = mSyntaxGraph;
    auto mr = g.get_allocator().resource();
    auto scratch = mScratch;

    Expects(g.isNamespace(mCurrentScope));
    auto parentID = locate(mCurrentScope, g);

    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Graph_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits0)), // trait
        std::forward_as_tuple(), // constraints
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    if (vertex == "_") {
        vertex = "";
    }
    if (edge == "_") {
        edge = "";
    }
    std::pmr::string vertexPropertyPath(vertex, scratch);
    std::pmr::string edgePropertyPath(edge, scratch);
    convertTypename(vertexPropertyPath);
    convertTypename(edgePropertyPath);

    auto& s = get_by_tag<Graph_>(vertID, g);
    s.mVertexProperty = g.getTypePath(mCurrentScope, vertexPropertyPath, mr, mr);
    s.mEdgeProperty = g.getTypePath(mCurrentScope, edgePropertyPath, mr, mr);

    {
        auto& traits = get(g.traits, g, vertID);
        if (g.isPmr(vertID)) {
            traits.mPmr |= true;
        }
    }

    auto handle = appendTypePath(*this, name, vertID);

    addAlias("vertex_descriptor", "");
    addAlias("edge_type", "");

    if (s.isVector()) {
        projectTypescript("vertex_descriptor", "number");
    } else {
        projectTypescript("vertex_descriptor",  std::string(name) + "Vertex");
    }

    return handle;
}

void ModuleBuilder::addGraphComponent(SyntaxGraph::vertex_descriptor vertID,
    std::string_view name, std::string_view type, std::string_view memberName) {
    auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    auto& s = get_by_tag<Graph_>(vertID, g);

    auto typeName = convertTypename(type, scratch);

    if (isInstance(typeName)) {
        g.instantiate(mCurrentScope, typeName, scratch);
    }

    auto valueID = g.lookupType(mCurrentScope, typeName, scratch);
    auto valuePath = g.getTypePath(valueID, g.get_allocator().resource());

    auto& c = s.mComponents.emplace_back();
    c.mName = name;
    c.mValuePath = valuePath;
    c.mMemberName = memberName;

    c.mVector = true;
    bool bPmr = g.isPmr(valueID) || g.isPmr(vertID);
    if (bPmr) {
        c.mContainerPath = "/ccstd/pmr/vector";
    } else {
        c.mContainerPath = "/boost/container/vector";
    }
}

void ModuleBuilder::addGraphPolymorphic(SyntaxGraph::vertex_descriptor vertID,
    std::string_view tag, std::string_view type, std::string_view memberName) {
    if (memberName == "_")
        memberName = "";

    auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    auto& s = get_by_tag<Graph_>(vertID, g);

    auto typeName = convertTypename(type, scratch);
    auto tagName = convertTypename(tag, scratch);

    if (isInstance(typeName)) {
        g.instantiate(mCurrentScope, typeName, scratch);
    }

    auto tagID = g.lookupType(mCurrentScope, tagName, scratch);
    auto tagPath = g.getTypePath(tagID, g.get_allocator().resource());

    auto valueID = g.lookupType(mCurrentScope, typeName, scratch);
    auto typePath = g.getTypePath(valueID, g.get_allocator().resource());

    auto& c = s.mPolymorphic.mConcepts.emplace_back();
    c.mTag = tagPath;
    c.mValue = typePath;
    c.mMemberName = memberName;
    bool bPmr = g.isPmr(valueID) || g.isPmr(vertID);
    if (bPmr) {
        c.mContainerPath = "/ccstd/pmr/vector";
    } else {
        c.mContainerPath = "/boost/container/vector";
    }
}

void ModuleBuilder::addVertexMap(SyntaxGraph::vertex_descriptor vertID,
    std::string_view mapType, std::string_view memberName,
    std::string_view keyType) {
    auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    auto& s = get_by_tag<Graph_>(vertID, g);

    auto mapName = convertTypename(mapType, scratch);
    auto keyName = convertTypename(keyType, scratch);

    if (isInstance(keyName)) {
        g.instantiate(mCurrentScope, keyName, scratch);
    }
    std::pmr::string typePath(scratch);
    {
        pmr_ostringstream oss(std::ios::out, scratch);
        oss << mapName << "<" << keyName << ",vertex_descriptor"
            << ">";
        typePath = oss.str();
        g.instantiate(mCurrentScope, typePath, scratch);
    }

    auto mapID = g.lookupType(mCurrentScope, mapName, scratch);
    auto mapPath = g.getTypePath(mapID, g.get_allocator().resource());

    auto keyID = g.lookupType(mCurrentScope, keyName, scratch);
    auto keyPath = g.getTypePath(keyID, g.get_allocator().resource());

    auto& map = s.mVertexMaps.emplace_back();
    map.mMapType = mapPath;
    map.mKeyType = keyPath;
    map.mMemberName = memberName;
    map.mTypePath = typePath;
}

void ModuleBuilder::addVertexBimap(SyntaxGraph::vertex_descriptor vertID,
    std::string_view mapType, std::string_view memberName,
    std::string_view componentName, std::string_view componentMemberName) {
    auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    auto& s = get_by_tag<Graph_>(vertID, g);

    auto mapName = convertTypename(mapType, scratch);
    std::pmr::string keyName(scratch);
    for (const auto& c : s.mComponents) {
        if (c.mName == componentName) {
            auto componentID = locate(c.mValuePath, g);
            if (componentMemberName.empty()) {
                keyName = g.getDependentName(mCurrentScope, componentID, scratch, scratch);
            } else {
                visit_vertex(
                    componentID, g,
                    [&](const Composition_ auto& s) {
                        for (const auto& m : s.mMembers) {
                            if (m.mMemberName == componentMemberName) {
                                auto memberID = locate(m.mTypePath, g);
                                keyName = g.getDependentName(mCurrentScope, memberID, scratch, scratch);
                                break;
                            }
                        }
                    },
                    [&](const auto&) {
                        throw std::out_of_range("cannot find member");
                    });
            }
            break;
        }
    }
    if (keyName.empty())
        throw std::out_of_range("component not found");

    std::pmr::string typePath(scratch);
    {
        pmr_ostringstream oss(std::ios::out, scratch);
        oss << mapName << "<" << keyName << ",vertex_descriptor"
            << ">";
        typePath = oss.str();
        g.instantiate(mCurrentScope, typePath, scratch);
    }

    auto mapID = g.lookupType(mCurrentScope, mapName, scratch);
    auto mapPath = g.getTypePath(mapID, g.get_allocator().resource());

    auto keyID = g.lookupType(mCurrentScope, keyName, scratch);
    auto keyPath = g.getTypePath(keyID, g.get_allocator().resource());

    auto& map = s.mVertexMaps.emplace_back();
    map.mMapType = mapPath;
    map.mKeyType = keyPath;
    map.mMemberName = memberName;
    map.mComponentName = componentName;
    map.mComponentMemberName = componentMemberName;
    map.mTypePath = typePath;
}

void ModuleBuilder::addNamedConcept(SyntaxGraph::vertex_descriptor vertID, bool bComponent,
    std::string_view componentName, std::string_view componentMemberName) {
    auto& g = mSyntaxGraph;
    auto scratch = mScratch;
    auto& s = get<Graph>(vertID, g);
    s.mNamed = true;
    s.mNamedConcept.mComponent = bComponent;
    s.mNamedConcept.mComponentName = componentName;
    s.mNamedConcept.mComponentMemberName = componentMemberName;
}

namespace {

void getIncluded(ModuleGraph::vertex_descriptor vertID,
    const ModuleGraph& mg,
    std::pmr::set<ModuleGraph::vertex_descriptor>& included) {
    const auto& m = get(mg.modules, mg, vertID);
    for (const auto& require : m.mRequires) {
        auto moduleID = locate(mg.null_vertex(), require, mg);
        included.emplace(moduleID);
        getIncluded(moduleID, mg, included);
    }
}

std::pmr::set<ModuleGraph::vertex_descriptor> getIndirectIncludes(
    ModuleGraph::vertex_descriptor vertID, const ModuleGraph& mg,
    std::pmr::memory_resource* scratch) {
    std::pmr::set<ModuleGraph::vertex_descriptor> included(scratch);
    const auto& m = get(mg.modules, mg, vertID);
    for (const auto& require : m.mRequires) {
        auto moduleID = locate(mg.null_vertex(), require, mg);
        getIncluded(moduleID, mg, included);
    }
    return included;
}

static const bool sReorder = true;

std::pmr::string reorderIncludes(std::pmr::string content,
    std::pmr::memory_resource* scratch) {
    if (!sReorder) {
        return std::pmr::string(content, scratch);
    }

    pmr_ostringstream oss(std::ios::out, scratch);

    std::istringstream iss(content);
    std::pmr::set<std::pmr::string> includes(scratch);
    std::pmr::set<std::pmr::string> includesHpp(scratch);
    std::pmr::set<std::pmr::string> includesStd(scratch);
    std::pmr::set<std::pmr::string> includesComma(scratch);
    std::pmr::string line(scratch);

    auto outputIncludes = [&]() {
        for (const auto& line : includes) {
            oss << line << "\n";
        }
        for (const auto& line : includesHpp) {
            oss << line << "\n";
        }
        for (const auto& line : includesStd) {
            oss << line << "\n";
        }
        for (const auto& line : includesComma) {
            oss << line << "\n";
        }
        includes.clear();
        includesHpp.clear();
        includesStd.clear();
        includesComma.clear();
    };

    while (std::getline(iss, line)) {
        const std::string_view include = "#include";
        if (!line.starts_with(include)) {
            outputIncludes();
            oss << line << "\n";
        } else {
            auto content = line.substr(include.size());
            boost::algorithm::trim(content);
            if (content.starts_with("<") && content.ends_with(">")) {
                if (content.ends_with(".hpp>")) {
                    includesHpp.emplace(line);
                } else if (content.ends_with(".h>")) {
                    includes.emplace(line);
                } else if (!boost::algorithm::contains(content, ".")) {
                    includesStd.emplace(line);
                }
            } else {
                includesComma.emplace(line);
            }
        }
    }
    outputIncludes();
    return oss.str();
}

void outputComment(std::ostream& oss) {
    oss << R"(/****************************************************************************
 Copyright (c) 2021-2022 Xiamen Yaji Software Co., Ltd.

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
****************************************************************************/

/**
 * ========================= !DO NOT CHANGE THE FOLLOWING SECTION MANUALLY! =========================
 * The following section is auto-generated.
 * ========================= !DO NOT CHANGE THE FOLLOWING SECTION MANUALLY! =========================
 */
)";

}

}

void ModuleBuilder::outputModule(std::string_view name, std::pmr::set<std::pmr::string>& files) const {
    auto scratch = mScratch;
    Expects(!name.empty());

    auto moduleID = locate(mModuleGraph.null_vertex(), name, mModuleGraph);
    if (moduleID == mModuleGraph.null_vertex())
        return;

    const auto& g = mSyntaxGraph;
    const auto& mg = mModuleGraph;

    const auto& m = get(mModuleGraph.modules, mModuleGraph, moduleID);
    auto modulePath = get_path(moduleID, mModuleGraph, scratch);
    Expects(!modulePath.empty());
    Expects(modulePath.front() == '/');

    const auto& moduleInfo = get(mg.modules, mg, moduleID);
    const auto features = moduleInfo.mFeatures;
    const auto& typescriptFolder = mTypescriptFolder;
    const auto& cppFolder = mCppFolder;
    auto ccFolder = std::string_view(m.mFolder);

    if (features & Features::Fwd) {
        std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Fwd.h" , scratch);
        std::filesystem::path filename = cppFolder / shortname;
        files.emplace(std::move(shortname));
        pmr_ostringstream oss(std::ios_base::out, scratch);
        std::pmr::string space(scratch);
        outputComment(oss);
        OSS << "// clang-format off\n";
        OSS << "#pragma once\n";
        const auto included = getIndirectIncludes(moduleID, mg, scratch);
        for (const auto& require : m.mRequires) {
            auto moduleID = locate(mg.null_vertex(), require, mg);
            if (included.contains(moduleID))
                continue;
            const auto& dep = get(mg.modules, mg, moduleID);
            auto ccDepFolder = std::string_view(dep.mFolder);
            if (dep.mFilePrefix.ends_with(".h")) {
                OSS << "#include \"" << ccDepFolder << "/" << dep.mFilePrefix << "\"\n";
            } else {
                OSS << "#include \"" << ccDepFolder << "/" << dep.mFilePrefix << "Fwd.h\"\n";
            }
        }
        if (mBoost) {
            OSS << "#include \"cocos/base/std/variant.h\"\n";
        }
        copyString(oss, generateFwd_h(mProjectName, mSyntaxGraph, mModuleGraph, modulePath, scratch, scratch));
        oss << "\n";
        OSS << "// clang-format on\n";

        updateFile(filename, reorderIncludes(oss.str(), scratch));
    }

    if (features & Features::Names) {
        std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Names.h", scratch);
        std::filesystem::path filename = cppFolder / shortname;
        files.emplace(std::move(shortname));
        pmr_ostringstream oss(std::ios_base::out, scratch);
        std::pmr::string space(scratch);
        outputComment(oss);
        OSS << "// clang-format off\n";
        OSS << "#pragma once\n";
        OSS << "#include \"" << ccFolder << "/" << m.mFilePrefix << "Types.h\"\n";
        const auto included = getIndirectIncludes(moduleID, mg, scratch);
        for (const auto& require : m.mRequires) {
            auto moduleID = locate(mg.null_vertex(), require, mg);
            if (included.contains(moduleID))
                continue;
            const auto& dep = get(mg.modules, mg, moduleID);
            auto ccDepFolder = std::string_view(dep.mFolder);
            if (!dep.mFilePrefix.ends_with(".h")) {
                OSS << "#include \"" << ccDepFolder << "/" << dep.mFilePrefix << "Names.h\"\n";
            }
        }
        copyString(oss, generateNames_h(mSyntaxGraph, mModuleGraph, modulePath, scratch, scratch));
        oss << "\n";
        OSS << "// clang-format on\n";
        updateFile(filename, reorderIncludes(oss.str(), scratch));
    }

    if (features & Features::Types) {
        std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Types.h", scratch);
        files.emplace(std::move(shortname));

        const std::filesystem::path filename = cppFolder / m.mFolder / m.mFilePrefix;

        {
            auto filename1 = filename;
            filename1 += "Types.h";

            pmr_ostringstream oss(std::ios_base::out, scratch);
            std::pmr::string space(scratch);
            outputComment(oss);
            OSS << "// clang-format off\n";
            OSS << "#pragma once\n";

            if (!moduleInfo.mAPI.empty()) {
                oss << "#include <" << std::filesystem::path(moduleInfo.mFolder).generic_string()
                    << "/Config.h>\n";
            }

            if (features & Features::Fwd) {
                OSS << "#include \"" << ccFolder << "/" << m.mFilePrefix << "Fwd.h\"\n";
            }
            const auto included = getIndirectIncludes(moduleID, mg, scratch);
            for (const auto& require : m.mRequires) {
                auto moduleID = locate(mg.null_vertex(), require, mg);
                if (included.contains(moduleID))
                    continue;
                const auto& dep = get(mg.modules, mg, moduleID);
                auto ccDepFolder = std::string_view(dep.mFolder);
                if (dep.mFilePrefix.ends_with(".h")) {
                    OSS << "#include \"" << ccDepFolder << "/" << dep.mFilePrefix << "\"\n";
                } else {
                    OSS << "#include \"" << ccDepFolder << "/" << dep.mFilePrefix << "Types.h\"\n";
                }
            }
            if (g.moduleHasContainer(modulePath, "/cc/IntrusivePtr")) {
                OSS << "#include \"cocos/base/Ptr.h\"\n";
            }
            if (g.moduleHasGraph(modulePath)) {
                OSS << "#include <boost/graph/graph_traits.hpp>\n";
                OSS << "#include <boost/graph/adjacency_iterator.hpp>\n";
                OSS << "#include <boost/graph/properties.hpp>\n";
                OSS << "#include <boost/range/irange.hpp>\n";
                OSS << "#include \"cocos/base/std/container/vector.h\"\n";
                OSS << "#include \"cocos/renderer/pipeline/custom/GraphTypes.h\"\n";
            }
            if (g.moduleHasMap(modulePath, "/ccstd/pmr/map")) {
                OSS << "#include \"base/std/container/map.h\"\n";
            }
            if (g.moduleHasMap(modulePath, "/cc/TransparentMap")
                || g.moduleHasMap(modulePath, "/cc/TransparentMultiMap")
                || g.moduleHasMap(modulePath, "/cc/PmrTransparentMap")
                || g.moduleHasMap(modulePath, "/cc/PmrTransparentMultiMap")
                || g.moduleHasMap(modulePath, "/cc/FlatMap")
                || g.moduleHasMap(modulePath, "/cc/FlatMultiMap")
                || g.moduleHasMap(modulePath, "/cc/PmrFlatMap")
                || g.moduleHasMap(modulePath, "/cc/PmrFlatMultiMap")
                || g.moduleHasMap(modulePath, "/cc/PmrUnorderedMap")
                || g.moduleHasMap(modulePath, "/cc/PmrUnorderedMultiMap")
                || g.moduleHasMap(modulePath, "/cc/UnorderedStringMap")
                || g.moduleHasMap(modulePath, "/cc/UnorderedStringMultiMap")
                || g.moduleHasMap(modulePath, "/cc/PmrUnorderedStringMap")
                || g.moduleHasMap(modulePath, "/cc/PmrUnorderedStringMultiMap")) {
                OSS << "#include \"cocos/renderer/pipeline/custom/Map.h\"\n";
            }
            if (g.moduleHasContainer(modulePath, "/cc/TransparentSet")
                || g.moduleHasContainer(modulePath, "/cc/TransparentMultiSet")
                || g.moduleHasContainer(modulePath, "/cc/PmrTransparentSet")
                || g.moduleHasContainer(modulePath, "/cc/PmrTransparentMultiSet")
                || g.moduleHasContainer(modulePath, "/cc/FlatSet")
                || g.moduleHasContainer(modulePath, "/cc/FlatMultiSet")
                || g.moduleHasContainer(modulePath, "/cc/PmrFlatSet")
                || g.moduleHasContainer(modulePath, "/cc/PmrFlatMultiSet")
                || g.moduleHasContainer(modulePath, "/cc/PmrUnorderedSet")
                || g.moduleHasContainer(modulePath, "/cc/PmrUnorderedMultiSet")
                || g.moduleHasContainer(modulePath, "/cc/UnorderedStringSet")
                || g.moduleHasContainer(modulePath, "/cc/UnorderedStringMultiSet")
                || g.moduleHasContainer(modulePath, "/cc/PmrUnorderedStringSet")
                || g.moduleHasContainer(modulePath, "/cc/PmrUnorderedStringMultiSet")) {
                OSS << "#include \"cocos/renderer/pipeline/custom/Set.h\"\n";
            }
            if (g.moduleHasType(modulePath, "/ccstd/pmr/string")) {
                OSS << "#include \"cocos/base/std/container/string.h\"\n";
            }
            if (g.moduleHasContainer(modulePath, "/boost/container/pmr/list")) {
                OSS << "#include <boost/container/pmr/list.hpp>\n";
            }
            if (g.moduleUsesHashCombine(modulePath)) {
                OSS << "#include \"cocos/base/std/hash/hash.h\"\n";
            }
            if (!moduleInfo.mHeader.empty()) {
                copyCppString(oss, space, moduleInfo.mHeader);
            }
            copyString(oss, generateTypes_h(mProjectName, mSyntaxGraph, mModuleGraph, modulePath, scratch, scratch));
            oss << "\n";
            OSS << "// clang-format on\n";
            updateFile(filename1, reorderIncludes(oss.str(), scratch));
        }

        {
            std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Types.cpp", scratch);
            files.emplace(std::move(shortname));

            auto filename1 = filename;
            filename1 += "Types.cpp";

            pmr_ostringstream oss(std::ios_base::out, scratch);
            std::pmr::string space(scratch);
            outputComment(oss);
            OSS << "// clang-format off\n";
            OSS << "#include \"" << m.mFilePrefix << "Types.h\"\n";

            copyString(oss, generateTypes_cpp(mProjectName, mSyntaxGraph, mModuleGraph, modulePath, scratch, scratch));
            oss << "\n";
            OSS << "// clang-format on\n";
            updateFile(filename1, reorderIncludes(oss.str(), scratch));
        }
    }
    if (features & Features::Graphs) {
        std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Graphs.h", scratch);
        files.emplace(std::move(shortname));

        std::filesystem::path filename = cppFolder / m.mFolder / m.mFilePrefix;
        filename += "Graphs.h";

        pmr_ostringstream oss(std::ios_base::out, scratch);
        std::pmr::string space(scratch);
        outputComment(oss);
        OSS << "// clang-format off\n";
        OSS << "#pragma once\n";
        OSS << "#include \"" << ccFolder << "/" << m.mFilePrefix << "Types.h\"\n";
        oss << "#include \"cocos/renderer/pipeline/custom/GraphImpl.h\"\n";
        oss << "#include \"cocos/renderer/pipeline/custom/Overload.h\"\n";
        oss << "#include \"cocos/renderer/pipeline/custom/PathUtils.h\"\n";
        oss << "#include <tuple>\n";
        oss << "#include <string_view>\n";

        copyString(oss, generateGraphs_h(mProjectName, mSyntaxGraph, mModuleGraph, modulePath, scratch, scratch));
        oss << "\n";
        OSS << "// clang-format on\n";
        updateFile(filename, reorderIncludes(oss.str(), scratch));
    }
    if (features & Features::Reflection) {
        {
            std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Reflection.h", scratch);
            files.emplace(std::move(shortname));

            std::filesystem::path filename = cppFolder / m.mFolder / m.mFilePrefix;
            filename += "Reflection.h";

            pmr_ostringstream oss(std::ios_base::out, scratch);
            std::pmr::string space(scratch);
            outputComment(oss);
            OSS << "// clang-format off\n";
            OSS << "#pragma once\n";

            if (!moduleInfo.mAPI.empty()) {
                oss << "#include <" << std::filesystem::path(moduleInfo.mFolder).generic_string()
                    << "/Config.h>\n";
            }
            OSS << "#include \"" << ccFolder << "/" << m.mFilePrefix << "Fwd.h\"\n";
            const auto included = getIndirectIncludes(moduleID, mg, scratch);
            for (const auto& require : m.mRequires) {
                auto moduleID = locate(mg.null_vertex(), require, mg);
                if (included.contains(moduleID))
                    continue;
                const auto& moduleInfo = get(mg.modules, mg, moduleID);
                const auto& dep = get(mg.modules, mg, moduleID);
                auto ccDepFolder = std::string_view(dep.mFolder);
                if (moduleInfo.mFeatures & Reflection) {
                    if (!dep.mFilePrefix.ends_with(".h")) {
                        OSS << "#include \"" << ccDepFolder << "/" << dep.mFilePrefix << "Reflection.h\"\n";
                    }
                }
            }
            copyString(oss, generateReflection_h(mProjectName, mSyntaxGraph, mModuleGraph, modulePath, scratch, scratch));
            oss << "\n";
            OSS << "// clang-format on\n";
            updateFile(filename, reorderIncludes(oss.str(), scratch));
        }
        {
            std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Reflection.cpp", scratch);
            files.emplace(std::move(shortname));

            std::filesystem::path filename = cppFolder / m.mFolder / m.mFilePrefix;
            filename += "Reflection.cpp";

            pmr_ostringstream oss(std::ios_base::out, scratch);
            std::pmr::string space(scratch);
            outputComment(oss);
            OSS << "// clang-format off\n";
            OSS << "#include \"" << m.mFilePrefix << "Reflection.h\"\n";
            OSS << "#include \"" << m.mFilePrefix << "Types.h\"\n";

            copyString(oss, generateReflection_cpp(mProjectName, mSyntaxGraph, mModuleGraph, modulePath, scratch, scratch));
            oss << "\n";
            OSS << "// clang-format on\n";
            updateFile(filename, reorderIncludes(oss.str(), scratch));
        }
    }

    if (features & Features::Jsb) {
        std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Jsb.h", scratch);
        files.emplace(std::move(shortname));
        const std::filesystem::path filename = cppFolder / m.mFolder / m.mFilePrefix;
        {
            auto filename1 = filename;
            filename1 += "Jsb.h";

            pmr_ostringstream oss(std::ios_base::out, scratch);
            std::pmr::string space(scratch);
            outputComment(oss);
            OSS << "// clang-format off\n";
            OSS << "#pragma once\n";

            if (!moduleInfo.mAPI.empty()) {
                oss << "#include <" << std::filesystem::path(moduleInfo.mFolder).generic_string()
                    << "/Config.h>\n";
            }

            OSS << "#include \"" << ccFolder << "/" << m.mFilePrefix << "Fwd.h\"\n";
            OSS << "#include \"cocos/bindings/manual/jsb_conversions.h\"\n";
            const auto included = getIndirectIncludes(moduleID, mg, scratch);
            for (const auto& require : m.mRequires) {
                auto moduleID = locate(mg.null_vertex(), require, mg);
                if (included.contains(moduleID))
                    continue;
                const auto& dep = get(mg.modules, mg, moduleID);
                auto ccDepFolder = std::string_view(dep.mFolder);
                if (!dep.mFilePrefix.ends_with(".h")) {
                    OSS << "#include \"" << ccDepFolder << "/" << dep.mFilePrefix << "Jsb.h\"\n";
                }
            }
            copyString(oss, generateJsbConversions_h(*this, moduleID));
            oss << "\n";
            OSS << "// clang-format on\n";
            updateFile(filename1, reorderIncludes(oss.str(), scratch));
        }

        {
            std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Jsb.cpp", scratch);
            files.emplace(std::move(shortname));

            auto filename1 = filename;
            filename1 += "Jsb.cpp";

            pmr_ostringstream oss(std::ios_base::out, scratch);
            std::pmr::string space(scratch);
            outputComment(oss);
            OSS << "// clang-format off\n";
            OSS << "#include \"" << ccFolder << "/" << m.mFilePrefix << "Jsb.h\"\n";
            OSS << "#include \"" << ccFolder << "/" << m.mFilePrefix << "Types.h\"\n";
            OSS << "#include \"cocos/renderer/pipeline/custom/JsbConversion.h\"\n";
            copyString(oss, space, m.mJsbHeaders);
            copyString(oss, generateJsbConversions_cpp(*this, moduleID));
            oss << "\n";
            OSS << "// clang-format on\n";
            updateFile(filename1, reorderIncludes(oss.str(), scratch));
        }
    }

    if (false && (features & Features::ToJs)) {
        // we must manually update
        // 1. CMakeLists
        // 2. tools/bindings-generator/conversions.yaml
        Expects(!m.mToJsFilename.empty());
        Expects(!m.mToJsPrefix.empty());
        Expects(!m.mToJsNamespace.empty());
        std::filesystem::path filename = cppFolder / "tools/tojs" / m.mToJsFilename;

        //pmr_ostringstream oss(std::ios_base::out, scratch);
        std::ostringstream oss;
        std::pmr::string space(scratch);
        
        copyString(oss, generateToJsIni(*this, moduleID));

        updateFile(filename, oss.str());
    }

    if (features & Features::ToJs) {
        Expects(!m.mToJsFilename.empty());
        Expects(!m.mToJsPrefix.empty());
        Expects(!m.mToJsNamespace.empty());
        std::filesystem::path filename = cppFolder / "tools/swig-config" / m.mToJsFilename;

        std::ostringstream oss;
        std::pmr::string space(scratch);

        copyString(oss, generateSwigConfig(*this, moduleID));

        updateFile(filename, oss.str());
    }

    if (features & Features::Typescripts) {
        std::filesystem::path tsPath = typescriptFolder / m.mTypescriptFolder / m.mTypescriptFilePrefix;
        std::filesystem::path filename = tsPath;
        filename += ".ts";
        pmr_ostringstream oss(std::ios_base::out, scratch);
        std::pmr::string space(scratch);
        CodegenContext codegen(scratch);
        codegen.mScopes.emplace_back("Struct");

        std::pmr::set<std::pmr::string> graphImports(scratch);
        int count = 0;
        {
            auto imported = g.getImportedTypes(modulePath, scratch);
            for (const auto& m : imported) {
                const auto targetID = locate(m.first, mModuleGraph);
                const auto targetPath = get_path(targetID, mg, scratch);
                const auto& target = get(mModuleGraph.modules, mModuleGraph, targetID);
                OSS << "import { ";
                int count = 0;
                for (const auto& type : m.second) {
                    if (count++)
                        oss << ", ";
                    auto vertID = locate(type, g);
                    auto tsName = g.getTypescriptTypename(type, scratch, scratch);
                    oss << tsName;
                    if ((features & Features::Serialization) && (target.mFeatures & Features::Serialization)) {
                        if (holds_tag<Struct_>(vertID, g)) {
                            oss << ", save" << tsName;
                            oss << ", load" << tsName;
                        }
                    }
                    const auto& traits = get(g.traits, g, vertID);
                    if (false && (traits.mFlags & EQUAL) && !(traits.mFlags & NO_EQUAL)) {
                        oss << ", equal" << tsName;
                    }
                }
                oss << " } from '";

                std::filesystem::path tsPath1 = typescriptFolder / target.mTypescriptFolder / target.mTypescriptFilePrefix;
                oss << getRelativePath(tsPath.generic_string(), tsPath1.generic_string(), scratch);
                oss << "';\n";
            }

            if (!m.mTypescriptInclude.empty()) {
                copyString(oss, space, m.mTypescriptInclude);
                ++count;
            }

            if (count || !imported.empty())
                oss << "\n";
        }

        for (const auto& vertID : make_range(vertices(g))) {
            const auto& typeModulePath = get(g.modulePaths, g, vertID);
            if (typeModulePath != modulePath)
                continue;
            outputTypescript(oss, space, codegen, *this, "", vertID, graphImports, scratch);
        }

        if (features & Features::Serialization) {
            copyString(oss, space, generateSerialization_ts(
                mProjectName, mSyntaxGraph, mModuleGraph, modulePath, false, scratch, scratch));
        }

        if (false) {
            OSS << "/*\n";
            OSS << "import {";
            for (int count = 0; const auto& vertID : make_range(vertices(g))) {
                const auto& typeModulePath = get(g.modulePaths, g, vertID);
                if (typeModulePath != modulePath)
                    continue;
                const auto& traits = get(g.traits, g, vertID);
                if (traits.mFlags & IMPL_DETAIL)
                    continue;

                const auto& name = get(g.names, g, vertID);

                bool bOutput = false;
                visit_vertex(
                    vertID, g,
                    [&](const Enum& e) {
                        bOutput = true;
                    },
                    [&](const Graph& s) {
                        if (count++) {
                            oss << ",\n";
                            OSS; 
                        }
                        oss << name;
                        if (!s.isVector()) {
                            oss << ", " << name << "Vertex";
                        }
                        count = 4;
                    },
                    [&](const Struct& s) {
                        bOutput = true;
                    },
                    [&](const Variant& s) {
                        bOutput = true;
                    },
                    [&](const auto&) {
                    });

                if (bOutput) {
                    if (count++)
                        oss << ",";
                    if (count % 5 == 0) {
                        oss << "\n";
                        OSS;
                    } else {
                        oss << " ";
                    }
                    oss << name;
                }
            }
            oss << " } from './" << m.mTypescriptFilePrefix << "';\n";
            OSS << "*/\n";
        }

        codegen.mScopes.pop_back();

        pmr_ostringstream oss2(std::ios_base::out, scratch);
        outputComment(oss2);
        oss2 << "/* eslint-disable max-len */\n";
        if (graphImports.empty()) {
            if (g.moduleHasGraph(modulePath)) {
                oss2 << "import * as impl from './graph';\n";
            }
        } else {
            oss2 << "import { ";
            int count = 0;
            for (const auto& name : graphImports) {
                if (count++) {
                    oss2 << ", ";
                }
                oss2 << name;
            }
            oss2 << " } from './graph';\n";
        }
        copyString(oss2, oss.str());
        updateFile(filename, oss2.str());
    }

    if (features & Features::Serialization) {
        std::pmr::string shortname(m.mFolder + "/" + m.mFilePrefix + "Serialization.h", scratch);
        std::filesystem::path filename = cppFolder / shortname;
        files.emplace(std::move(shortname));
        pmr_ostringstream oss(std::ios_base::out, scratch);
        std::pmr::string space(scratch);
        outputComment(oss);
        OSS << "#pragma once\n";
        OSS << "#include \"" << ccFolder << "/" << m.mFilePrefix << "Types.h\"\n";
        const auto included = getIndirectIncludes(moduleID, mg, scratch);
        for (const auto& require : m.mRequires) {
            auto moduleID = locate(mg.null_vertex(), require, mg);
            if (included.contains(moduleID))
                continue;
            const auto& dep = get(mg.modules, mg, moduleID);
            auto ccDepFolder = std::string_view(dep.mFolder);
            if (!dep.mFilePrefix.ends_with(".h")) {
                OSS << "#include \"" << ccDepFolder << "/" << dep.mFilePrefix << "Serialization.h\"\n";
            }
        }
        if (features & Features::Graphs && g.moduleHasGraphSerialization(modulePath)) {
            OSS << "#include <" << m.mFolder << "/" << m.mFilePrefix << "Graphs.h>\n";
        }
        OSS << "#include \"" << ccFolder << "/ArchiveTypes.h\"\n";
        OSS << "#include \"" << ccFolder << "/SerializationUtils.h\"\n";
        OSS << "#include \"" << ccFolder << "/Range.h\"\n";
        copyString(oss, generateSerialization_h(mProjectName, mSyntaxGraph,
            mModuleGraph, modulePath, false, scratch, scratch));
        updateFile(filename, reorderIncludes(oss.str(), scratch));
    }
}

void ModuleBuilder::addTypescriptFunctions(SyntaxGraph::vertex_descriptor vertID, std::string_view content) {
    auto& g = mSyntaxGraph;
    visit_vertex(
        vertID, g,
        [&](Composition_ auto& s) {
            s.mTypescriptFunctions.emplace_back(content);
        },
        [&](const auto&) {
        });
}

void ModuleBuilder::projectTypescript(std::string_view cpp, std::string_view ts) {
    auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    std::pmr::string path(cpp, scratch);
    convertTypename(path);

    std::pmr::string tsName(ts, scratch);
    removeParenthesis(tsName);
    
    if (isInstance(path)) {
        g.instantiate(mCurrentScope, path, scratch);
    }

    auto vertID = g.lookupType(mCurrentScope, path, scratch);
    Expects(vertID != g.null_vertex());

    auto& tsInfo = get(g.typescripts, g, vertID);
    tsInfo.mName = tsName;
}

// Generation
int ModuleBuilder::compile() {
    auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    // resolve all member types
    for (auto vertID : make_range(vertices(g))) {
        auto ns = mSyntaxGraph.getNamespace(vertID, scratch);
        visit_vertex(
            vertID, g,
            [&](Composition_ auto& s) {
                for (Member& m : s.mMembers) {
                    Expects(!m.mTypePath.empty());
                    if (!isTypePath(m.mTypePath)) {
                        auto memberID = g.lookupType(ns, m.mTypePath, scratch);
                        if (memberID == g.null_vertex()) {
                            throw std::out_of_range("type cannot be resolved");
                        } else {
                            m.mTypePath = g.getTypePath(memberID, g.get_allocator().resource());
                        }
                    }
                }
            },
            [&](const auto&) {
            });
    }

    // complete graphs
    for (auto vertID : make_range(vertices(g))) {
        if (!holds_alternative<Graph>(vertID, g))
            continue;

        { // complete vertex list
            auto& s = get<Graph>(vertID, g);
            if (g.isPmr(vertID)) {
                auto& traits = get(g.traits, g, vertID);
                traits.mPmr |= true;
                if (s.mVertexListPath.empty()) {
                    s.mVertexListPath = "/ccstd/pmr/vector";
                }
                if (s.mEdgeListPath.empty()) {
                    s.mEdgeListPath = "/cc/PmrList";
                }
                if (s.mOutEdgeListPath.empty()) {
                    s.mOutEdgeListPath = "/ccstd/pmr/vector";
                }
            } else {
                if (s.mVertexListPath.empty()) {
                    s.mVertexListPath = "/std/vector";
                }
                if (s.mEdgeListPath.empty()) {
                    s.mEdgeListPath = "/std/list";
                }
                if (s.mOutEdgeListPath.empty()) {
                    s.mOutEdgeListPath = "/std/vector";
                }
            }
        }

        // copy graph, vertex might be invalidated
        Graph s(get<Graph>(vertID, g), scratch);
        mCurrentScope = g.getScope(vertID, scratch);
        mCurrentModule = get(g.modulePaths, g, vertID);
        { // add alias
            if (s.mIncidence) {
                addAlias("OutEdge", "");
                if (s.mBidirectional) {
                    addAlias("InEdge", "");
                }
            }
            if (s.mReferenceGraph && !s.mAliasGraph) {
                addAlias("ChildEdge", "");
                addAlias("ParentEdge", "");
            }
            if (s.isPolymorphic()) {
                addAlias("VertexHandle", "");
            }
        }

        const auto& moduleID = locate(mCurrentModule, mModuleGraph);
        const auto& m = get(mModuleGraph.modules, mModuleGraph, moduleID);
        auto ns = mSyntaxGraph.getNamespace(vertID, mScratch);

        CppGraphBuilder builder(&mSyntaxGraph, &mModuleGraph,
            vertID, moduleID,
            ns, !m.mAPI.empty(), mProjectName, scratch);

        auto bPmr = g.isPmr(vertID);
        if (s.mReferenceGraph && !s.mAliasGraph) {
            auto scope = addStruct(
                "Object",
                Traits{
                    .mPmr = bPmr,
                    .mFlags = NO_SERIALIZATION | IMPL_DETAIL,
                });
            auto objectID = scope.mVertexDescriptor;
            addMember(objectID, true, builder.childListType(), "mChildren");
            addMember(objectID, true, builder.parentListType(), "mParents");
        }
        {
            auto scope = addStruct(
                "Vertex",
                Traits{
                    .mPmr = bPmr && s.mIncidence,
                    .mFlags = NO_SERIALIZATION | IMPL_DETAIL,
                });
            auto vertexID = scope.mVertexDescriptor;

            if (s.mIncidence) {
                addMember(vertexID, true, builder.outEdgeListType(), "mOutEdges");
                if (s.mBidirectional) {
                    addMember(vertexID, true, builder.inEdgeListType(), "mInEdges");
                }
            }
            if (s.hasVertexProperty()) {
                addMember(vertexID, true, builder.vertexPropertyType(), "mProperty");
                addConstructor(vertexID, { "mProperty" }, false);
            }
            if (s.isPolymorphic()) {
                addMember(vertexID, true, "VertexHandle", "mHandle");
            }
        }

        std::pmr::vector<Member> members(g.get_allocator());
        {
            auto& s0 = get<Graph>(vertID, g);
            members = std::move(s0.mMembers);
            s0.mMembers.clear();
        }

        if (s.mReferenceGraph && !s.mAliasGraph) {
            addMember(vertID, true, builder.objectListType(), "mObjects", "_",
                NO_SERIALIZATION | IMPL_DETAIL | NOT_ELEMENT, R"(// Owners
)");
        }

        addMember(vertID, false, builder.vertexListType(), "mVertices", "_",
            NO_SERIALIZATION | IMPL_DETAIL | NOT_ELEMENT, R"(// Vertices
)");

        for (size_t i = 0; i != s.mComponents.size(); ++i) {
            const auto& c = s.mComponents[i];
            std::string_view comments;
            if (i == 0)
                comments = R"(// Components
)";
            auto memberID = locate(c.mValuePath, g);
            pmr_ostringstream oss(std::ios::out, scratch);
            auto typeName = g.getDependentName(mCurrentScope, memberID, scratch, scratch);
            auto listID = locate(builder.componentContainerType(), g);
            oss << g.getDependentName(mCurrentScope, listID, scratch, scratch);
            oss << "<" << typeName << ">";
            addMember(vertID, true, oss.str(), c.mMemberName,
                "_", NO_SERIALIZATION | IMPL_DETAIL, comments);
        }

        for (size_t i = 0, count = 0; i != s.mPolymorphic.mConcepts.size(); ++i) {
            const auto& c = s.mPolymorphic.mConcepts[i];
            if (c.mMemberName.empty())
                continue;
            std::string_view comments;
            if (count++ == 0)
                comments = R"(// PolymorphicGraph
)";
            auto conceptID = locate(c.mValue, g);
            pmr_ostringstream oss(std::ios::out, scratch);
            auto typeName = g.getDependentName(mCurrentScope, conceptID, scratch, scratch);
            auto listID = g.null_vertex();
            if (bPmr) {
                listID = c.isVector() ? locate("/ccstd/pmr/vector", g)
                                      : locate("/cc/PmrList", g);
            } else {
                listID = c.isVector() ? locate("/boost/container/vector", g)
                                      : locate("/boost/container/list", g);
            }
            Ensures(listID != g.null_vertex());
            oss << g.getDependentName(mCurrentScope, listID, scratch, scratch);
            oss << "<" << typeName << ">";
            addMember(vertID, true, oss.str(), c.mMemberName,
                "_", NO_SERIALIZATION | IMPL_DETAIL, comments);
        }

        if (s.needEdgeList()) {
            addMember(vertID, true, builder.edgeListType(), "mEdges", "_",
                NO_SERIALIZATION | IMPL_DETAIL | NOT_ELEMENT,
                R"(// Edges
)");
        }

        for (int count = 0; const auto& map : s.mVertexMaps) {
            std::string_view comments;
            if (count++ == 0)
                comments = R"(// UuidGraph
)";
            addMember(vertID, true, map.mTypePath, map.mMemberName,
                "_", NO_SERIALIZATION | IMPL_DETAIL | NOT_ELEMENT, comments);
        }

        int count = 0;
        for (auto& s = get<Graph>(vertID, g); auto& m : members) {
            if (count++ == 0)
                m.mComments = "// Members\n";
            s.mMembers.emplace_back(std::move(m));
        }

        if (s.mAddressable) {
            visit(
                overload(
                    [&](Map_) {
                        std::pmr::string indexName(scratch);
                        if (bPmr) {
                            if (s.mAddressableConcept.mUtf8) {
                                indexName = "PmrTransparentMap<ccstd::pmr::u8string, vertex_descriptor>";
                            } else {
                                indexName = "PmrTransparentMap<ccstd::pmr::string, vertex_descriptor>";
                            }
                        } else {
                            if (s.mAddressableConcept.mUtf8) {
                                indexName = "TransparentMap<ccstd::u8string, vertex_descriptor>";
                            } else {
                                indexName = "TransparentMap<ccstd::string, vertex_descriptor>";
                            }
                        }
                        addMember(vertID, true, indexName,
                            s.mAddressableConcept.mMemberName,
                            "_", NO_SERIALIZATION | IMPL_DETAIL | NOT_ELEMENT, "// Path\n");
                    },
                    [&](auto) {
                    }),
                s.mAddressableConcept.mType);
        }
    }
    mCurrentScope.clear();
    mCurrentModule.clear();

    mCompiled = true;
    return 0;
}

std::pmr::string ModuleBuilder::getTypedMemberName(
    const Member& m, bool bPublic, bool bFull) const {
    const auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    auto memberID = locate(m.mTypePath, g);
    auto typeName = g.getTypescriptTypename(memberID, scratch, scratch);

    auto name = g.getMemberName(m.mMemberName, bPublic);

    if (bFull || !g.isTypescriptData(typeName)) {
        name += ": ";
        name += typeName;
        if (m.mTypescriptOptional) {
            name += " | null";
        }
    }

    return name;
}

std::pmr::string ModuleBuilder::getTypedParameterName(const Parameter& p,
    bool bPublic, bool bFull, bool bOptional) const {
    const auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    auto memberID = locate(p.mTypePath, g);
    auto typeName = g.getTypescriptTypename(memberID, scratch, scratch);
    Expects(!typeName.empty());

    std::pmr::string result(scratch);

    if (bFull || !g.isTypescriptData(typeName)) {
        result += ": ";
        result += typeName;
        if (bOptional) {
            // currently, reference is not supported
            // we must use pointer
            result += " | null";
        }
    }

    return result;
}

std::pmr::string ModuleBuilder::getTypescriptVertexName(SyntaxGraph::vertex_descriptor vertID,
    std::string_view descName) const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios_base::out, scratch);
    const auto& g = mSyntaxGraph;
    const auto& s = get<Graph>(vertID, g);
    const auto& builder = *this;

    Expects(s.mNamed);
    Expects(s.mNamedConcept.mComponent);
    for (const auto& c : s.mComponents) {
        if (c.mName != s.mNamedConcept.mComponentName)
            continue;

        if (s.mNamedConcept.mComponentMemberName.empty()) {
            if (s.isVector()) {
                oss << "this." << g.getMemberName(c.mMemberName, false)
                    << "[" << descName << "]";
            } else {
                oss << s.getTypescriptVertexDereference(descName, scratch)
                    << "." << g.getMemberName(c.mMemberName, false);
            }
        } else {
            if (s.isVector()) {
                oss << "this." << g.getMemberName(c.mMemberName, false)
                    << "[" << descName << "]." << s.mNamedConcept.mComponentMemberName;
            } else {
                oss << s.getTypescriptVertexDereference(descName, scratch)
                    << "." << g.getMemberName(c.mMemberName, false)
                    << "." << s.mNamedConcept.mComponentMemberName;
            }
        }
    }

    return oss.str();
}

}
