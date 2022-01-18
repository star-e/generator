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
#include "SyntaxGraphs.h"
#include <Cocos/Path.h>
#include "SyntaxUtils.h"
#include "TypescriptBuilder.h"
#include "BuilderMacros.h"
#include "CppBuilder.h"

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

SyntaxGraph::vertex_descriptor ModuleBuilder::addContainer(std::string_view name, Traits traits) {
    auto& g = mSyntaxGraph;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Container_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits)), // trait
        std::forward_as_tuple(), // requires
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
        std::forward_as_tuple(), // requires
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
        std::forward_as_tuple(), // requires
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
        std::forward_as_tuple(), // requires
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    return vertID;
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addFlag(std::string_view name, Traits traits) {
    auto& g = mSyntaxGraph;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Enum_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits)), // trait
        std::forward_as_tuple(), // requires
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    get<Enum>(vertID, g).mIsFlags = true;

    return vertID;
}

void ModuleBuilder::addEnumElement(SyntaxGraph::vertex_descriptor vertID,
    std::string_view name, std::string_view value) {
    auto& g = mSyntaxGraph;
    Expects(holds_tag<Enum_>(vertID, g));

    auto& e = get_by_tag<Enum_>(vertID, g);
    EnumValue v(e.get_allocator());
    v.mName = name;
    v.mValue = value;
    e.mValues.emplace_back(std::move(v));
}

SyntaxGraph::vertex_descriptor ModuleBuilder::addTag(std::string_view name, bool bEntity,
    std::initializer_list<std::string_view> concepts) {
    auto& g = mSyntaxGraph;

    auto parentID = locate(mCurrentScope, g);
    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Tag_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(), // trait
        std::forward_as_tuple(), // requires
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(Tag{ .mEntity = bEntity }), // polymorphic
        g, parentID);

    return vertID;
}

TypeHandle ModuleBuilder::addStruct(std::string_view name, Traits traits) {
    auto& g = mSyntaxGraph;

    auto parentID = locate(mCurrentScope, g);
    Expects(parentID == SyntaxGraph::null_vertex()
        || holds_tag<Namespace_>(parentID, g)
        || holds_tag<Struct_>(parentID, g));

    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Struct_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits)), // trait
        std::forward_as_tuple(), // requires
        std::forward_as_tuple(), // inherits
        std::forward_as_tuple(mCurrentModule), // module path
        std::forward_as_tuple(), // typescript
        std::forward_as_tuple(), // polymorphic
        g, parentID);

    return appendTypePath(*this, name, vertID);
}

void ModuleBuilder::addInherits(SyntaxGraph::vertex_descriptor vertID, std::string_view className) {
    auto& g = mSyntaxGraph;
    auto scratch = get_allocator().resource();

    std::pmr::string adlPath(className, scratch);
    convertTypename(adlPath);

    if (isInstance(adlPath)) {
        g.instantiate(mCurrentScope, adlPath, scratch);
    }

    visit_vertex(
        vertID, g,
        [&](Composition_ auto& s) {
            auto vertID = g.lookupType(mCurrentScope, adlPath, scratch);
            auto typeName = g.getTypePath(vertID, g.get_allocator().resource());
            auto& inherits = get(g.inherits, g, vertID);
            inherits.mBases.emplace_back(std::move(typeName));
        },
        [&](const auto&) {
            Expects(false);
        });
}

void ModuleBuilder::addMember(SyntaxGraph::vertex_descriptor vertID, bool bPublic,
    std::string_view className, std::string_view memberName,
    std::string_view initial, GenerationFlags flags) {
    auto& g = mSyntaxGraph;
    auto scratch = get_allocator().resource();

    std::pmr::string adlPath(className, scratch);
    convertTypename(adlPath);

    memberName = boost::algorithm::trim_copy(memberName);
    if (initial == "_") {
        initial = {};
    }

    bool bInstance = isInstance(adlPath);
    if (bInstance) {
        g.instantiate(mCurrentScope, adlPath, scratch);
    }

    visit_vertex(
        vertID, g,
        [&](Composition_ auto& s) {
            Member m(get_allocator());
            std::pmr::string typeName(adlPath, scratch);
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
            Expects(vertID != g.null_vertex());

            auto typePath = g.getTypePath(vertID, scratch);

            m.mTypePath = std::move(typePath);
            m.mPublic = bPublic;
            m.mMemberName = memberName;
            m.mDefaultValue = initial;
            m.mFlags = flags;

            s.mMembers.emplace_back(std::move(m));
        },
        [&](const auto&) {
            // do nothing
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
        std::forward_as_tuple(), // requires
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
    Expects(holds_tag<Variant_>(vertID, g));
    auto& var = get<Variant>(vertID, g);
    var.mVariants.emplace_back(g.getTypePath(mCurrentScope, type,
        g.get_allocator().resource(), mScratch));
}

TypeHandle ModuleBuilder::addGraph(std::string_view name,
    std::string_view vertex, std::string_view edge, Traits traits) {
    auto& g = mSyntaxGraph;
    auto mr = g.get_allocator().resource();
    auto scratch = mScratch;

    Expects(g.isNamespace(mCurrentScope));
    auto parentID = locate(mCurrentScope, g);

    Expects(locate(parentID, name, g) == g.null_vertex());

    auto vertID = add_vertex(Graph_{},
        std::forward_as_tuple(name), // name
        std::forward_as_tuple(std::move(traits)), // trait
        std::forward_as_tuple(), // requires
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

    return appendTypePath(*this, name, vertID);
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
    auto tagPath = g.getTypePath(valueID, g.get_allocator().resource());

    auto& c = s.mComponents.emplace_back();
    c.mName = name;
    c.mValuePath = tagPath;
    c.mMemberName = memberName;
}

void ModuleBuilder::addGraphPolymorphic(SyntaxGraph::vertex_descriptor vertID,
    std::string_view tag, std::string_view type, std::string_view memberName) {
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
}

void ModuleBuilder::outputModule(const std::filesystem::path& rootFolder,
    std::string_view name, Features features) const {
    auto scratch = mScratch;
    Expects(!name.empty());

    auto moduleID = locate(mModuleGraph.null_vertex(), name, mModuleGraph);
    if (moduleID == mModuleGraph.null_vertex())
        return;

    const auto& g = mSyntaxGraph;

    const auto& moduleName = get(mModuleGraph.names, mModuleGraph, moduleID);
    const auto& m = get(mModuleGraph.modules, mModuleGraph, moduleID);
    auto modulePath = get_path(moduleID, mModuleGraph, scratch);
    Expects(!modulePath.empty());
    Expects(modulePath.front() == '/');

    if (features & Features::Typescripts) {
        std::filesystem::path tsPath = rootFolder / m.mFolder / m.mFilePrefix;
        std::filesystem::path filename = tsPath;
        filename += ".ts";
        pmr_ostringstream oss(std::ios_base::out, scratch);
        std::pmr::string space(scratch);
        CodegenContext codegen(scratch);
        codegen.mScopes.emplace_back("Struct");

        {
            int count = 0;
            if (g.moduleHasGraph(modulePath)) {
                ++count;
                OSS << "import * as impl from './graph';\n";
            }

            auto imported = g.getImportedTypes(modulePath, scratch);
            for (const auto& m : imported) {
                OSS << "import { ";
                int count = 0;
                for (const auto& type : m.second) {
                    if (count++)
                        oss << ", ";
                    oss << extractName(type);
                }
                oss << " } from '";

                const auto targetID = locate(m.first, mModuleGraph);
                const auto& target = get(mModuleGraph.modules, mModuleGraph, targetID);
                std::filesystem::path tsPath1 = rootFolder / target.mFolder / target.mFilePrefix;
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
            outputTypescript(oss, space, codegen, * this, "", vertID, scratch);
        }

        if (true) {
            OSS << "/*\n";
            OSS << "import {";
            for (int count = 0; const auto& vertID : make_range(vertices(g))) {
                const auto& typeModulePath = get(g.modulePaths, g, vertID);
                if (typeModulePath != modulePath)
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
            oss << " } from './" << m.mFilePrefix << "';\n";
            OSS << "*/\n";
        }

        codegen.mScopes.pop_back();
        updateFile(filename, oss.str());
    }

    if (features & Features::Fwd) {
        std::filesystem::path filename = rootFolder / m.mFolder / m.mFilePrefix;
        filename += "Fwd.h";
        pmr_ostringstream oss(std::ios_base::out, scratch);
        std::pmr::string space(scratch);
        OSS << "#pragma once\n";
        copyString(oss, generateFwd_h(mSyntaxGraph, modulePath, scratch, scratch));

        updateFile(filename, oss.str());
    }

    if (features & Features::Types) {
        std::filesystem::path filename = rootFolder / m.mFolder / m.mFilePrefix;
        filename += "Types.h";
        pmr_ostringstream oss(std::ios_base::out, scratch);
        std::pmr::string space(scratch);
        OSS << "#pragma once\n";
        copyString(oss, generateTypes_h(mSyntaxGraph, modulePath, scratch, scratch));

        updateFile(filename, oss.str());
    }
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
std::pmr::string ModuleBuilder::getMemberName(std::string_view memberName, bool bPublic) const {
    Expects(memberName.size() >= 2);

    std::pmr::string name(mScratch);
    if (mUnderscoreMemberName) {
        if (bPublic) {
            name = memberName.substr(1);
            name[0] = tolower(name[0]);
        } else {
            name = memberName;
            name[0] = '_';
            name[1] = tolower(name[1]);
        }
    } else {
        name = memberName;
    }
    return name;
}

std::pmr::string ModuleBuilder::getTypedMemberName(
    const Member& m, bool bPublic, bool bFull) const {
    const auto& g = mSyntaxGraph;
    auto scratch = mScratch;

    auto memberID = locate(m.mTypePath, g);
    auto typeName = g.getTypescriptTypename(memberID, scratch, scratch);

    auto name = getMemberName(m.mMemberName, bPublic);

    if (bFull || !g.isTypescriptData(typeName)) {
        name += ": ";
        name += typeName;
        if (m.mPointer) {
            name += " | null";
        }
    }

    return name;
}

}
