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
#include "SyntaxUtils.h"

namespace Cocos::Meta {

CppStructBuilder::CppStructBuilder(const allocator_type& alloc)
    : mCurrentNamespace(alloc)
    , mCurrentPath(alloc)
    , mName(alloc)
    , mAPI(alloc)
    , mProjectName(alloc) {}

CppStructBuilder::CppStructBuilder(const SyntaxGraph* syntaxGraph, const ModuleGraph* moduleGraph,
    uint32_t currentVertex, uint32_t currentModule,
    std::string_view currentNamespace,
    std::string_view projectName, 
    const allocator_type& alloc)
    : mSyntaxGraph(syntaxGraph)
    , mModuleGraph(moduleGraph)
    , mCurrentModule(currentModule)
    , mCurrentVertex(currentVertex)
    , mCurrentNamespace(currentNamespace, alloc)
    , mCurrentPath(alloc)
    , mName(alloc)
    , mAPI(alloc)
    , mProjectName(projectName, alloc) {
    const auto& g = *mSyntaxGraph;
    auto* scratch = get_allocator().resource();
    mCurrentPath = g.getTypePath(mCurrentVertex, scratch);
    mName = getDependentPath(mCurrentNamespace, mCurrentPath);
    const auto& mod = get(mModuleGraph->modules, *mModuleGraph, mCurrentModule);
    mAPI = mod.mAPI;
}

const Traits& CppStructBuilder::getTraits() const {
    const auto& g = *mSyntaxGraph;
    return get(g.traits, g, mCurrentVertex);
}

std::pmr::string CppStructBuilder::getDependentName(SyntaxGraph::vertex_descriptor vertID) const {
    const auto& g = *mSyntaxGraph;
    auto* scratch = get_allocator().resource();
    return getCppPath(mSyntaxGraph->getDependentName(mCurrentNamespace, vertID, scratch, scratch), scratch);
}

std::pmr::string CppStructBuilder::getDependentName(std::string_view typePath) const {
    const auto& g = *mSyntaxGraph;
    auto* scratch = get_allocator().resource();
    Expects(!typePath.empty());
    auto vertID = g.null_vertex();
    if (typePath.front() == '/') {
        vertID = locate(typePath, g);
    } else {
        vertID = g.lookupType(mCurrentNamespace, typePath, scratch);
    }
    Ensures(vertID != g.null_vertex());
    return getDependentName(vertID);
}

std::pmr::string CppStructBuilder::getImplName(std::string_view ns) const {
    auto* scratch = get_allocator().resource();
    const auto& g = *mSyntaxGraph;
    std::pmr::string cn(scratch);
    if (ns != ".") {
        cn = ns;
    } else {
        cn = g.getNamespace(mCurrentVertex, scratch);
    }
    return getCppPath(g.getDependentName(cn, mCurrentVertex, scratch, scratch), scratch);
}

std::pmr::string CppStructBuilder::generateGetAllocatorSignature(bool bInline) const {
    pmr_ostringstream oss(std::ios_base::out, get_allocator());
    if (!bInline) {
        oss << mName << "::";
    }
    oss << "allocator_type ";
    if (!bInline) {
        oss << mName << "::";
    }
    oss << "get_allocator() const noexcept";

    return oss.str();
}

std::pmr::string CppStructBuilder::generateGetAllocatorBody() const {
    pmr_ostringstream oss(std::ios_base::out, get_allocator());
    std::pmr::string space(get_allocator());

    const auto& g = *mSyntaxGraph;
    visit_vertex(
        mCurrentVertex, g,
        [&](const Composition_ auto& s) {
            bool found = false;
            for (const auto& m : s.mMembers) {
                auto vertID = locate(m.mTypePath, g);
                if (g.isOptional(vertID)) {
                    continue;
                }
                if (g.isPmr(vertID)) {
                    found = true;
                    OSS << "return allocator_type(" << m.mMemberName << ".get_allocator().resource());\n";
                    break;
                }
            }
            // struct only contains optional<pmr>, which is not enough
            Expects(found);
        },
        [&](const auto&) {
        });

    return oss.str();
}

namespace {

template<class T>
void outputMembers(std::ostream& oss, std::pmr::string& space,
    const SyntaxGraph& g, std::string_view ns,
    SyntaxGraph::vertex_descriptor vertID, const T& s,
    std::pmr::memory_resource* scratch) {
    for (const Member& m : s.mMembers) {
        auto memberID = locate(m.mTypePath, g);
        const auto& traits = get(g.traits, g, memberID);
        if (!m.mComments.empty()) {
            copyString(oss, space, m.mComments);
        }
        if (traits.mUnknown) {
            visit_vertex(
                memberID, g,
                [&](const Struct& s) {
                    OSS << "struct {\n";
                    {
                        INDENT();
                        outputMembers(oss, space, g, ns, memberID, s, scratch);
                    }
                    OSS << "} " << m.mMemberName << ";\n";
                },
                [&](const auto&) {
                    Expects(false);
                });
        } else {
            auto name = g.getDependentName(ns, memberID, scratch, scratch);
            if (name.empty()) {
                // if dependent name is empty, the scope is the same of input type
                name = get(g.names, g, vertID);
            }
            boost::algorithm::replace_all(name, "/", "::");
            OSS;
            if (m.mConst) {
                oss << "const ";
            }
            if (memberID == vertID) {
                if (traits.mClass) {
                    oss << "class ";
                } else {
                    oss << "struct ";
                }
            }
            oss << name;
            if (m.mPointer) {
                oss << "*";
            }
            oss << " " << m.mMemberName;
            if (!m.mDefaultValue.empty()) {
                oss << " = " << m.mDefaultValue;
            }
            oss << ";\n";
        }
    }
}

}

std::pmr::string CppStructBuilder::generateMembers() const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios_base::out, scratch);
    std::pmr::string space(scratch);
    const auto& g = *mSyntaxGraph;
    auto scope = g.getScope(mCurrentVertex, scratch);
    visit_vertex(
        mCurrentVertex, g,
        [&](const Composition_ auto& s) {
            outputMembers(oss, space, g, scope, mCurrentVertex, s, scratch);
        },
        [&](const auto&) {
            Expects(false);
        });

    return oss.str();
}

std::pmr::string CppStructBuilder::generateOperatorSignature(OperatorType type, bool bInline) const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios_base::out, scratch);
    std::pmr::string space(scratch);
    const auto& g = *mSyntaxGraph;

    const auto vertID = mCurrentVertex;
    const auto& name = get(g.names, g, vertID);
    const auto& trait = get(g.traits, g, vertID);

    auto outputRelation = [&](std::string_view op) {
        visit_vertex(
            vertID, g,
            [&](const Composition_ auto& s) {
                if (trait.mClass) {
                    oss << "bool operator" << op << "(const " << name << "& rhs) const noexcept";
                } else {
                    if (bInline) {
                        oss << "inline ";
                    }
                    oss << "bool operator" << op << "(const " << name << "& lhs, const " << name << "& rhs) noexcept";
                }
            },
            [&](const Tag& s) {
                if (bInline) {
                    oss << "inline ";
                }
                oss << "bool operator" << op << "(const " << name << "&, const " << name << "&) noexcept";
            },
            [&](const Variant& s) {
                if (bInline) {
                    oss << "inline ";
                }
                oss << "bool operator" << op << "(const " << name << "& lhs, const " << name << "& rhs) noexcept";
            },
            [&](const auto&) {
            });
    };

    switch (type) {
    case OperatorType::CopyAssign: {
        visit_vertex(
            vertID, g,
            [&](const Composition_ auto& s) {
                Expects(false);
                if (bInline) {
                    //OSS << name << ""
                }
            },
            [&](const auto&) {
                Expects(false);
            });

        break;
    }
    case OperatorType::MoveAssign: {

        break;
    }
    case OperatorType::Equal: {
        outputRelation("==");
        break;
    }
    case OperatorType::Unequal: {
        outputRelation("!=");
        break;
    }
    case OperatorType::Less: {
        outputRelation("<");
        break;
    }
    case OperatorType::Spaceship: {
        outputRelation("<=>");
        break;
    }
    case OperatorType::Hash: {
        Expects(false);
        break;
    }
    default:
        throw std::invalid_argument("unknown OperatorType case");
    }

    return oss.str();
}

std::pmr::string CppStructBuilder::generateOperatorBody(OperatorType type) const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios_base::out, scratch);
    std::pmr::string space(scratch);
    const auto& g = *mSyntaxGraph;

    const auto vertID = mCurrentVertex;
    auto name = get(g.names, g, vertID);
    const auto& trait = get(g.traits, g, vertID);

    std::string_view op;

    switch (type) {
    case OperatorType::Equal: {
        op = "==";
        break;
    }
    case OperatorType::Unequal: {
        OSS << "return !(lhs == rhs);\n";
        return oss.str();
    }
    case OperatorType::Less: {
        op = "<";
        break;
    }
    case OperatorType::Spaceship: {
        op = "<=>";
        break;
    }
    default:
        Expects(false);
    }

    OSS << "return std::forward_as_tuple(";
    visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            int count = 0;
            std::string v = "lhs.";
            for (const Member& m : s.mMembers) {
                if (count++)
                    oss << ", ";
                oss << v << m.mMemberName;
            }
            oss << ")\n";
            OSS << "    " << op << " std::forward_as_tuple(";
            v = "rhs.";
            count = 0;
            for (const Member& m : s.mMembers) {
                if (count++)
                    oss << ", ";
                oss << v << m.mMemberName;
            }
            oss << ");\n";
        },
        [&](const auto&) {
            // do nothing
        });

    return oss.str();
}

namespace {

template<Composition_ T>
void generateCntr(std::ostream& oss, std::pmr::string& space,
    const CppStructBuilder& cpp, const SyntaxGraph& g,
    const T& s, const Constructor& cntr,
    std::pmr::memory_resource* scratch) {
    INDENT();
    for (uint32_t i = 0, count = 0; const auto& m : s.mMembers) {
        auto memberID = locate(m.mTypePath, g);
        bool bPmr = g.isPmr(memberID);
        if (m.mReference || m.mPointer)
            bPmr = false;
        bool isParam = false;
        isParam = std::find(cntr.mIndices.begin(), cntr.mIndices.end(), i) != cntr.mIndices.end();

        const auto paramName = getMemberName(m.mMemberName, scratch);

        if (isParam) {
            if (count++) {
                oss << "\n";
                OSS << ", ";
            } else {
                OSS << ": ";
            }
            if (g.isOptional(memberID)) {
                if (bPmr) {
                    oss << m.mMemberName << "(" << paramName
                        << " ? " << cpp.getDependentName(memberID)
                        << "(std::in_place, std::move(*" << paramName
                        << "), alloc) : std::nullopt)";
                } else {
                    oss << m.mMemberName << "(std::move(" << paramName << "))";
                }
            } else {
                oss << m.mMemberName << "(std::move(" << paramName << ")";
                if (bPmr) {
                    oss << ", alloc)";
                } else {
                    oss << ")";
                }
            }
        } else {
            if (!g.isOptional(memberID)) {
                if (bPmr) {
                    if (count++) {
                        oss << "\n";
                        OSS << ", ";
                    } else {
                        OSS << ": ";
                    }
                    oss << m.mMemberName << "(alloc)";
                }
            }
        }
        ++i;
    }
    oss << " {}\n";
}

template <Composition_ T>
void generateMove(std::ostream& oss, std::pmr::string& space,
    const CppStructBuilder& cpp, const SyntaxGraph& g, const T& s,
    std::pmr::memory_resource* scratch) {
    INDENT();
    int count = 0;
    const auto optionalID = locate("/std/optional", g);
    for (const auto& m : s.mMembers) {
        auto memberID = locate(m.mTypePath, g);
        bool bPmr = g.isPmr(memberID);
        if (m.mReference || m.mPointer)
            bPmr = false;
        if (count++) {
            oss << "\n";
            OSS << ", ";
        } else {
            OSS << ": ";
        }

        std::string_view lhs = m.mMemberName;
        std::pmr::string rhs("rhs.", scratch);
        rhs.append(m.mMemberName);
        if (g.isInstantiation(memberID)) {
            auto templateID = g.getTemplate(memberID, scratch);
            if (templateID == optionalID) {
                const auto& inst = get<Instance>(memberID, g);
                Expects(inst.mParameters.size() == 1);
                auto parameterID = locate(inst.mParameters.front(), g);
                bPmr = g.isPmr(parameterID);

                if (bPmr) {
                    oss << lhs << "(" << rhs << " ? " << cpp.getDependentName(memberID)
                        << "(std::in_place, std::move(*" << rhs << "), alloc) : std::nullopt)";
                } else {
                    oss << lhs << "(std::move(" << rhs << "))";
                }
                continue;
            }
        }

        oss << m.mMemberName << "(std::move(rhs." << m.mMemberName << ")";
        if (bPmr) {
            oss << ", alloc)";
        } else {
            oss << ")";
        }
    }
    oss << " {}\n";
}

template <Composition_ T>
void generateCopy(std::ostream& oss, std::pmr::string& space,
    const CppStructBuilder& cpp, const SyntaxGraph& g, const T& s,
    std::pmr::memory_resource* scratch) {
    INDENT();
    int count = 0;
    const auto optionalID = locate("/std/optional", g);
    for (const auto& m : s.mMembers) {
        auto memberID = locate(m.mTypePath, g);
        bool bPmr = g.isPmr(memberID);
        if (m.mReference || m.mPointer)
            bPmr = false;
        if (count++) {
            oss << "\n";
            OSS << ", ";
        } else {
            OSS << ": ";
        }

        std::string_view lhs = m.mMemberName;
        std::pmr::string rhs("rhs.", scratch);
        rhs.append(m.mMemberName);
        if (g.isInstantiation(memberID)) {
            auto templateID = g.getTemplate(memberID, scratch);
            if (templateID == optionalID) {
                const auto& inst = get<Instance>(memberID, g);
                Expects(inst.mParameters.size() == 1);
                auto parameterID = locate(inst.mParameters.front(), g);
                bPmr = g.isPmr(parameterID);
                if (bPmr) {
                    oss << lhs << "(" << rhs << " ? " << cpp.getDependentName(memberID)
                        << "(std::in_place, *" << rhs << ", alloc) : std::nullopt)";
                } else {
                    oss << lhs << "(" << rhs << ")";
                }
                continue;
            }
        }

        oss << m.mMemberName << "(rhs." << m.mMemberName;
        if (bPmr) {
            oss << ", alloc)";
        } else {
            oss << ")";
        }
    }
    oss << " {}\n";
}

template <Composition_ T>
void generateMoveAssign(std::ostream& oss, std::pmr::string& space,
    const CppStructBuilder& cpp, const SyntaxGraph& g, const T& s,
    std::pmr::memory_resource* scratch) {
    OSS << "const auto alloc = get_allocator();\n";
    const auto optionalID = locate("/std/optional", g);
    for (const auto& m : s.mMembers) {
        auto memberID = locate(m.mTypePath, g);
        bool bPmr = g.isPmr(memberID);
        if (m.mReference || m.mPointer)
            bPmr = false;

        std::string_view lhs = m.mMemberName;
        std::pmr::string rhs("rhs.", scratch);
        rhs.append(m.mMemberName);
        if (g.isInstantiation(memberID)) {
            auto templateID = g.getTemplate(memberID, scratch);
            if (templateID == optionalID) {
                const auto& inst = get<Instance>(memberID, g);
                Expects(inst.mParameters.size() == 1);
                auto parameterID = locate(inst.mParameters.front(), g);
                bPmr = g.isPmr(parameterID);

                if (bPmr) {
                    OSS << lhs << " = " << rhs << " ? " << cpp.getDependentName(memberID)
                        << "(std::in_place, std::move(*" << rhs << "), alloc) : std::nullopt;\n";
                } else {
                    OSS << lhs << " = std::move(" << rhs << ");\n";
                }
                continue;
            }
        }
        OSS << lhs << " = std::move(" << rhs << ");\n";
    }
}

template <Composition_ T>
void generateCopyAssign(std::ostream& oss, std::pmr::string& space,
    const CppStructBuilder& cpp, const SyntaxGraph& g, const T& s,
    std::pmr::memory_resource* scratch) {
    OSS << "const auto alloc = get_allocator();\n";
    const auto optionalID = locate("/std/optional", g);
    for (const auto& m : s.mMembers) {
        auto memberID = locate(m.mTypePath, g);
        bool bPmr = g.isPmr(memberID);
        if (m.mReference || m.mPointer)
            bPmr = false;

        std::string_view lhs = m.mMemberName;
        std::pmr::string rhs("rhs.", scratch);
        rhs.append(m.mMemberName);
        if (g.isInstantiation(memberID)) {
            auto templateID = g.getTemplate(memberID, scratch);
            if (templateID == optionalID) {
                const auto& inst = get<Instance>(memberID, g);
                Expects(inst.mParameters.size() == 1);
                auto parameterID = locate(inst.mParameters.front(), g);
                bPmr = g.isPmr(parameterID);
                if (bPmr) {
                    OSS << lhs << " = " << rhs << " ? " << cpp.getDependentName(memberID)
                        << "(std::in_place, *" << rhs << ", alloc) : std::nullopt;\n";
                } else {
                    OSS << lhs << " = " << rhs << ";\n";
                }
                continue;
            }
        }
        OSS << lhs << " = " << rhs << ";\n";
    }
}

}

std::pmr::string CppStructBuilder::generateHeaderConstructors() const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios_base::out, scratch);
    std::pmr::string space(scratch);
    const auto& g = *mSyntaxGraph;

    const auto vertID = mCurrentVertex;
    auto name = get(g.names, g, vertID);
    const auto& traits = get(g.traits, g, vertID);

    bool bDLL = false;
    bool bPmr = g.isPmr(vertID);
    bool bNoexcept = g.isNoexcept(vertID);
    bool bDerived = !get(g.inherits, g, vertID).mBases.empty();

    std::pmr::string api(mAPI, scratch);
    if (!mAPI.empty()) {
        api.append("_API ");
        bDLL = true;
    }

    // Default
    auto needDefault = g.needDefaultCntr(vertID);
    if (bPmr) {
        if (needDefault == ImplEnum::Inline || needDefault == ImplEnum::Separated) {
            oss << "\n";
            OSS << api << name << "(const allocator_type& alloc";
            if (traits.mFlags & GenerationFlags::PMR_DEFAULT) {
                oss << " = std::pmr::get_default_resource()";
            }
            oss << ")";
            if (bNoexcept) {
                oss << " noexcept";
            }
        }
        switch (needDefault) {
        case ImplEnum::Inline: {
            oss << "\n";
            visit_vertex(
                vertID, g,
                [&](const Composition_ auto& s) {
                    Constructor cntr(scratch);
                    generateCntr(oss, space, *this, g, s, cntr, scratch);
                },
                [&](const auto&) {
                });
            break;
        }
        case ImplEnum::Separated:
            oss << ";\n";
            break;
        case ImplEnum::None:
        case ImplEnum::Delete:
        default:
            break;
        }
    } else {
        switch (needDefault) {
        case ImplEnum::Inline: {
            OSS << name << "() = default;\n";
            break;
        }
        case ImplEnum::Separated:
            if (bNoexcept) {
                OSS << api << name << "() noexcept";
            } else {
                OSS << api << name << "()";
            }
            if (!bDLL && traits.mInterface) {
                oss << " = default";
            }
            oss << ";\n";
            break;
        case ImplEnum::Delete:
            OSS << name << "() = delete;\n";
            break;
        case ImplEnum::None:
        default:
            break;
        }
    }

    // Custom Constructors
    visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            for (int count = 0; const auto& cntr : s.mConstructors) {
                const bool bNoDefaultCntr = traits.mFlags & GenerationFlags::NO_DEFAULT_CNTR;
                if (bPmr && bNoDefaultCntr && count++ == 0) {
                    oss << "\n";
                }
                if (bPmr) {
                    OSS << generateConstructorSignature(cntr, true) << ";\n";
                } else {
                    OSS << generateConstructorSignature(cntr, true) << "\n";
                    generateCntr(oss, space, *this, g, s, cntr, scratch);
                }
            }
            
            auto needMove = g.needMoveCntr(vertID);
            if (bPmr) {
                switch (needMove) {
                case ImplEnum::Inline:
                    OSS << name << "(" << name << "&& rhs, const allocator_type& alloc)\n";
                    generateMove(oss, space, *this, g, s, scratch);
                    break;
                case ImplEnum::Separated:
                    OSS << api << name << "(" << name << "&& rhs, const allocator_type& alloc);\n";
                    break;
                case ImplEnum::Delete:
                    OSS << name << "(" << name << "&& rhs) = delete;\n";
                    break;
                case ImplEnum::None:
                default:
                    break;
                }
            } else {
                switch (needMove) {
                case ImplEnum::Inline:
                    OSS << name << "(" << name << "&& rhs)";
                    if (bNoexcept)
                        oss << " noexcept";
                    oss << "\n";
                    generateMove(oss, space, *this, g, s, scratch);
                    break;
                case ImplEnum::Separated:
                    OSS << api << name << "(" << name << "&& rhs)";
                    if (bNoexcept)
                        oss << " noexcept";
                    oss << ";\n";
                    break;
                case ImplEnum::Delete:
                    OSS << name << "(" << name << "&& rhs) = delete;\n";
                    break;
                case ImplEnum::None:
                default:
                    break;
                }
            }

            auto needCopy = g.needCopyCntr(vertID);
            if (bPmr) {
                switch (needCopy) {
                case ImplEnum::Inline:
                    OSS << name << "(" << name << " const& rhs, const allocator_type& alloc)\n";
                    generateCopy(oss, space, *this, g, s, scratch);
                    break;
                case ImplEnum::Separated:
                    OSS << api << name << "(" << name << " const& rhs, const allocator_type& alloc);\n";
                    break;
                case ImplEnum::Delete:
                case ImplEnum::None:
                default:
                    break;
                }
            } else {
                switch (needCopy) {
                case ImplEnum::Inline:
                    OSS << name << "(" << name << " const& rhs)";
                    oss << "\n";
                    generateCopy(oss, space, *this, g, s, scratch);
                    break;
                case ImplEnum::Separated:
                    OSS << api << name << "(" << name << " const& rhs)";
                    oss << ";\n";
                    break;
                case ImplEnum::Delete:
                    OSS << name << "(" << name << " const& rhs) = delete;\n";
                    break;
                case ImplEnum::None:
                default:
                    break;
                }
            }

            if (bPmr) {
                if (needMove == ImplEnum::Separated) {
                    oss << "\n";
                    if (bDLL) {
                        OSS << api << name << "(" << name << "&& rhs)";
                        if (bNoexcept) {
                            oss << " noexcept";
                        }
                        oss << ";\n";
                    } else {
                        OSS << name << "(" << name << "&& rhs) = default;\n";
                    }
                }
                if (needMove == ImplEnum::Inline) {
                    OSS << name << "(" << name << "&& rhs) = default;\n";
                }
                if (needCopy != ImplEnum::None) {
                    OSS << name << "(" << name << " const& rhs) = delete;\n";
                }
            }

            switch (needMove) {
            case ImplEnum::Inline:
            case ImplEnum::Separated:
                if (bDLL) {
                    OSS << api << name << "& operator=(" << name << "&& rhs);\n";
                } else {
                    OSS << name << "& operator=(" << name << "&& rhs) = default;\n";
                }
                break;
            case ImplEnum::Delete:
                OSS << name << "& operator=(" << name << "&& rhs) = delete;\n";
                break;
            case ImplEnum::None:
            default:
                break;
            }

            switch (needCopy) {
            case ImplEnum::Inline:
            case ImplEnum::Separated:
                if (bDLL) {
                    OSS << api << name << "& operator=(" << name << " const& rhs);\n";
                } else {
                    OSS << name << "& operator=(" << name << " const& rhs) = default;\n";
                }
                break;
            case ImplEnum::Delete:
                OSS << name << "& operator=(" << name << " const& rhs) = delete;\n";
                break;
            case ImplEnum::None:
            default:
                break;
            }

            auto needDtor = g.needDtor(vertID, bDLL);
            switch (needDtor) {
            case ImplEnum::Inline:
            case ImplEnum::Separated:
                if (traits.mInterface) {
                    if (bDerived) {
                        OSS << api << "~" << name << "() noexcept override = 0;\n";
                    } else {
                        OSS << api << "virtual ~" << name << "() noexcept = 0";
                        if (bDLL) {
                            oss << ";\n";
                        } else {
                            oss << " {}\n";
                        }
                    }
                } else {
                    if (bDerived) {
                        OSS << api << "~" << name << "() noexcept override;\n";
                    } else {
                        OSS << api << "~" << name << "() noexcept;\n";
                    }
                }
                break;
            case ImplEnum::None:
            case ImplEnum::Delete:
            default:
                break;
            }
        },
        [&](const auto&) {
        });

    return oss.str();
}

std::pmr::string CppStructBuilder::generateCppConstructors() const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios_base::out, scratch);
    std::pmr::string space(scratch);
    const auto& g = *mSyntaxGraph;

    const auto vertID = mCurrentVertex;
    auto name = get(g.names, g, vertID);
    auto structName = getImplName();
    const auto& traits = get(g.traits, g, vertID);

    bool bDLL = !mAPI.empty();
    bool bPmr = g.isPmr(vertID);
    bool bNoexcept = g.isNoexcept(vertID);

    // Custom Constructors
    visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            if (bDLL && bPmr) {
                oss << "\n";
                OSS << structName << "::allocator_type " << structName << "::get_allocator() const noexcept {\n";
                {
                    INDENT();
                    copyString(oss, space, generateGetAllocatorBody());
                }
                OSS << "}\n";
            }

            bool needNewLine = false;
            // Default
            if (!(traits.mFlags & CUSTOM_CNTR)) {
                auto needDefault = g.needDefaultCntr(vertID);
                if (needDefault == ImplEnum::Separated) {
                    if (bPmr) {
                        oss << "\n";
                        OSS << structName << "::" << name << "(const allocator_type& alloc)";
                        if (bNoexcept) {
                            oss << " noexcept";
                        }
                        oss << "\n";
                        {
                            Constructor cntr(scratch);
                            generateCntr(oss, space, *this, g, s, cntr, scratch);
                        }
                        needNewLine = true;
                    } else if (traits.mInterface && bDLL) {
                        oss << "\n";
                        OSS << structName << "::" << name << "()";
                        if (bNoexcept) {
                            oss << " noexcept";
                        }
                        oss << " = default;\n";
                    }
                }
                for (const auto& cntr : s.mConstructors) {
                    if (bPmr) {
                        oss << "\n";
                        OSS << generateConstructorSignature(cntr, false) << "\n";
                        generateCntr(oss, space, *this, g, s, cntr, scratch);
                        needNewLine = true;
                    }
                }
            }

            auto needMove = g.needMoveCntr(vertID);
            if (needMove == ImplEnum::Separated) {
                oss << "\n";
                OSS << structName << "::" << name << "(" << name << "&& rhs, const allocator_type& alloc)\n";
                generateMove(oss, space, *this, g, s, scratch);
                needNewLine = true;
            }

            auto needCopy = g.needCopyCntr(vertID);
            if (needCopy == ImplEnum::Separated) {
                oss << "\n";
                OSS << structName << "::" << name << "(" << name << " const& rhs, const allocator_type& alloc)\n";
                generateCopy(oss, space, *this, g, s, scratch);
                needNewLine = true;
            }

            

            if (bDLL && needMove == ImplEnum::Separated) {
                oss << "\n";
                OSS << structName << "::" << name << "(" << name << "&& rhs)";
                if (bNoexcept) {
                    oss << " noexcept";
                }
                oss << " = default;\n";
                needNewLine = false;
            }

            const bool hasPmrOptional = g.hasPmrOptional(vertID);
            if (bDLL && needMove == ImplEnum::Separated) {
                if (bPmr && hasPmrOptional) {
                    oss << "\n";
                    OSS << structName << "& " << structName << "::operator=(" << name << "&& rhs) {\n";
                    {
                        INDENT();
                        generateMoveAssign(oss, space, *this, g, s, scratch);
                        OSS << "return *this;\n";
                    }
                    OSS << "}\n";
                    needNewLine = true;
                } else {
                    OSS << structName << "& " << structName << "::operator=(" << name << "&& rhs) = default;\n";
                    needNewLine = false;
                }
            }

            if (bDLL && needCopy == ImplEnum::Separated) {
                if (bPmr && hasPmrOptional) {
                    oss << "\n";
                    OSS << structName << "& " << structName << "::operator=(" << name << " const& rhs) {\n";
                    {
                        INDENT();
                        generateCopyAssign(oss, space, *this, g, s, scratch);
                        OSS << "return *this;\n";
                    }
                    OSS << "}\n";
                    needNewLine = true;
                } else {
                    OSS << structName << "& " << structName << "::operator=(" << name << " const& rhs) = default;\n";
                    needNewLine = false;
                }
            }

            auto needDtor = g.needDtor(vertID, bDLL);
            if (bDLL && needDtor == ImplEnum::Separated) {
                if (!(traits.mFlags & GenerationFlags::CUSTOM_DTOR)) {
                    if (hasPmrOptional || needNewLine) {
                        oss << "\n";
                    }
                    OSS << structName << "::~" << name << "() noexcept = default;\n";
                }
            }
        },
        [&](const auto&) {
        });

    return oss.str();
}

std::pmr::string CppStructBuilder::generateConstructorSignature(
    const Constructor& cntr, bool bInline) const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios_base::out, scratch);
    std::pmr::string space(scratch);
    const auto& g = *mSyntaxGraph;

    const auto vertID = mCurrentVertex;
    auto name = get(g.names, g, vertID);
    const auto& traits = get(g.traits, g, vertID);
    const auto bPmr = g.isPmr(vertID);
    const auto bDLL = !mAPI.empty();
    bool bNoexcept = g.isNoexcept(vertID);
    if (bInline) {
        if (bDLL && bPmr)
            oss << mAPI << "_API ";
    } else {
        oss << name << "::";
    }

    oss << name << "(";

    visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            int count = 0;
            for (const auto& k : cntr.mIndices) {
                if (count++)
                    oss << ", ";
                const auto& m = s.mMembers.at(k);
                auto memberID = locate(m.mTypePath, g);
                if (m.mTypePath == "/std/pmr/string") {
                    bNoexcept = false;
                    oss << "std::string_view " << getMemberName(m.mMemberName, scratch);
                } else if (m.mTypePath == "/std/pmr/u8string") {
                    bNoexcept = false;
                    oss << "std::u8string_view " << getMemberName(m.mMemberName, scratch);
                } else {
                    auto name = g.getDependentName(mCurrentNamespace, memberID, scratch, scratch);
                    oss << getCppPath(name, scratch);
                    if (m.mPointer) {
                        oss << "*";
                    }
                    if (m.mReference) {
                        oss << "&";
                    }
                    oss << " " << getMemberName(m.mMemberName, scratch);
                }
            }
            if (g.isPmr(vertID)) {
                if (count++)
                    oss << ", ";
                oss << "const allocator_type& alloc";
                if (bInline && (traits.mFlags & GenerationFlags::PMR_DEFAULT)) {
                    oss << " = std::pmr::get_default_resource()";
                }
            }
        },
        [&](const auto&) {

        });

    oss << ")";

    if (bNoexcept && !(traits.mFlags & CUSTOM_CNTR)) {
        oss << " noexcept";
    }

    return oss.str();
}

std::pmr::string CppStructBuilder::generateConstructorBody(const Constructor& cntr) const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios_base::out, scratch);
    std::pmr::string space(scratch);
    const auto& g = *mSyntaxGraph;

    const auto vertID = mCurrentVertex;
    auto name = get(g.names, g, vertID);
    const auto& traits = get(g.traits, g, vertID);

    return oss.str();
}

std::pmr::string CppStructBuilder::generateMemberFunctions(std::pmr::string& space) const {
    auto scratch = get_allocator().resource();
    pmr_ostringstream oss(std::ios_base::out, scratch);

    const auto& g = *mSyntaxGraph;
    const auto vertID = mCurrentVertex;
    auto name = get(g.names, g, vertID);
    const auto& traits = get(g.traits, g, vertID);

    std::pmr::string api(mAPI, scratch);
    if (!api.empty())
        api.append("_API ");

    visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            for (const auto& func : s.mMemberFunctions) {
                auto functions = boost::algorithm::replace_all_copy(
                    func, "[[no_sender]] ", api);
                functions = boost::algorithm::replace_all_copy(
                    functions, "[[sender]] ", api);
                functions = boost::algorithm::replace_all_copy(
                    functions, "[[dll]] ", api);
                copyCppString(oss, space, functions);
            }
        },
        [&](const auto&) {
        });

    return oss.str();
}

}