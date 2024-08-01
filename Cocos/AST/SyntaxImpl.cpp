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
#include "SyntaxGraphs.h"
#include "SyntaxUtils.h"
#include "TypescriptGraph.h"
#include "CodeConfigs.h"

namespace Cocos::Meta {

std::pmr::string Member::getMemberName() const {
    Expects(mMemberName.size() > 1);
    if (mPublic) {
        return camelToVariable(mMemberName.substr(1), get_allocator().resource());
    } else {
        auto name = camelToVariable(mMemberName.substr(1), get_allocator().resource());
        name.insert(name.begin(), '_');
        return name;
    }
}

std::pmr::string Member::getTypescriptMemberName() const {
    if (mTypescriptMemberName.empty()) {
        return getMemberName();
    }
    if (mPublic) {
        return mTypescriptMemberName;
    }
    return "_" + mTypescriptMemberName;
}

std::pmr::string SyntaxGraph::getMemberName(std::string_view memberName, bool bPublic) const {
    Expects(memberName.size() >= 2);
    std::pmr::string name(mScratch);
    if (bPublic) {
        name = camelToVariable(memberName.substr(1), mScratch);
    } else {
        name = camelToVariable(memberName.substr(1), mScratch);
        name.insert(name.begin(), '_');
    }
    return name;
}

bool SyntaxGraph::isNamespace(std::string_view typePath) const noexcept {
    const auto& g = *this;
    if (typePath.empty()) {
        return true;
    }
    auto parentID = locate(typePath, g);
    return parentID != g.null_vertex();
}

bool SyntaxGraph::isInterface(vertex_descriptor vertID) const noexcept {
    auto scratch = mScratch;
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);
    return traits.mInterface;
}

bool SyntaxGraph::isIntrusivePtr(vertex_descriptor vertID) const noexcept {
    auto scratch = mScratch;
    const auto& g = *this;
    if (g.isInstantiation(vertID)) {
        auto typePath = g.getTypePath(vertID, scratch);
        std::pmr::string container(scratch);
        std::pmr::vector<std::pmr::string> parameters(scratch);
        extractTemplate(typePath, container, parameters);

        if (container == "/cc/IntrusivePtr") {
            Expects(parameters.size() == 1);
            return true;
        }
    }
    return false;
}

bool SyntaxGraph::isValueType(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    if (holds_tag<Value_>(vertID, g)
        || holds_tag<Enum_>(vertID, g)
        || isTag(vertID)) {
        return true;
    }
    const auto& traits = get(g.traits, g, vertID);
    if (traits.mTrivial) {
        return true;
    }
    if (holds_tag<Alias_>(vertID, g)) {
        const auto& alias = get<Alias>(vertID, g);
        if (!alias.mTypePath.empty()) {
            auto typeID = locate(alias.mTypePath, g);
            if (g.isValueType(typeID)) {
                return true;
            }
        }
    }
    return false;
}

bool SyntaxGraph::isValueType(const Parameter& param) const {
    if (param.mPointer || param.mReference) {
        return true;
    }
    const auto paramID = locate(param.mTypePath, *this);
    return isValueType(paramID);
}

bool SyntaxGraph::isInstantiation(vertex_descriptor vertID) const noexcept {
    if (holds_tag<Instance_>(vertID, *this)) {
        return true;
    }
    return false;
}

bool SyntaxGraph::isInstanceDependent(vertex_descriptor vertID) const noexcept {
    for (const auto& g = *this; vertID != null_vertex(); vertID = parent(vertID, g)) {
        if (holds_tag<Instance_>(vertID, g)) {
            return true;
        }
    }
    return false;
}

bool SyntaxGraph::isTag(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    if (holds_tag<Tag_>(vertID, g))
        return true;

    if (holds_tag<Variant_>(vertID, g)) {
        const auto& var = get_by_tag<Variant_>(vertID, g);
        for (const auto& param : var.mVariants) {
            if (param.mTypePath == "/std/monostate") {
                continue;
            }
            auto typeID = locate(param.mTypePath, g);
            if (!holds_tag<Tag_>(typeID, g)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool SyntaxGraph::isPmr(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);
    if (traits.mPmr)
        return true;

    auto checkStruct = [&](const Composition_ auto& s) {
        bool bPmr = false;
        for (const auto& m : s.mMembers) {
            auto memberID = locate(m.mTypePath, g);
            if (memberID == vertID)
                continue;

            if (m.mPointer || m.mReference)
                continue;

            if (g.isPmr(memberID))
                bPmr = true;
        }
        return bPmr;
    };

    return visit_vertex(
        vertID, g,
        [&](const Struct& s) {
            return checkStruct(s);
        },
        [&](const Graph& s) {
            bool bPmr = checkStruct(s);
            for (const auto& c : s.mComponents) {
                auto componentID = locate(c.mValuePath, g);
                Expects(componentID != vertID);
                if (g.isPmr(componentID))
                    bPmr = true;
            }
            for (const auto& c : s.mPolymorphic.mConcepts) {
                auto objectID = locate(c.mValue, g);
                Expects(objectID != vertID);
                if (g.isPmr(objectID))
                    bPmr = true;
            }
            return bPmr;
        },
        [&](const Instance& s) {
            const auto optionalID = locate("/std/optional", g);
            auto templateID = locate(s.mTemplate, g);
            const auto& traits = get(g.traits, g, templateID);

            if (traits.mPmr)
                return true;

            if (s.mTemplate == "/std/shared_ptr"
                || s.mTemplate == "/std/weak_ptr")
                return false;

            if (templateID == optionalID) {
                Expects(s.mParameters.size() == 1);
                auto parameterID = locate(s.mParameters.front(), g);
                return g.isPmr(parameterID);
            }

            for (const auto& typePath : s.mParameters) {
                const auto typeInfo = extractType(typePath);
                if (typeInfo.mPointer) {
                    return false;
                }
                auto paramID = locate(typeInfo.mShortName, g);
                const auto& traits = get(g.traits, g, paramID);
                Expects(!traits.mPmr);
            }
            return false;
        },
        [&](const auto&) {
            return false;
        });
}

bool SyntaxGraph::isNoexcept(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);

    if (!traits.mNoexcept) {
        return false;
    }

    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            bool bThrow = false;
            for (const auto& m : s.mMembers) {
                auto memberID = locate(m.mTypePath, g);
                if (memberID == vertID)
                    continue;

                if (!g.isNoexcept(memberID))
                    bThrow = true;
            }
            if (g.hasConcreteBase(vertID)) {
                bThrow = true;
            }
            return !bThrow;
        },
        [&](const Instance& s) {
            auto templateID = locate(s.mTemplate, g);
            const auto& traits = get(g.traits, g, templateID);
            bool bThrow = false;
            if (!traits.mNoexcept)
                bThrow = true;

            for (const auto& typePath : s.mParameters) {
                auto typeInfo = extractType(typePath);
                if (typeInfo.mPointer) {
                    continue;
                }
                auto paramID = locate(typePath, g);
                const auto& traits = get(g.traits, g, paramID);
                if (!traits.mNoexcept)
                    bThrow = true;
            }
            return !bThrow;
        },
        [&](const auto&) {
            return true;
        });
}

bool SyntaxGraph::isSerializable(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);
    if (traits.mFlags & GenerationFlags::NO_SERIALIZATION)
        return false;

    return true;
}

bool SyntaxGraph::isSerializable(vertex_descriptor vertID, const Member& member) const noexcept {
    return isSerializable(vertID) && !(member.mFlags & NO_SERIALIZATION);
}

bool SyntaxGraph::isComposition(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto&) {
            return true;
        },
        [&](const auto&) {
            return false;
        });
}

bool SyntaxGraph::isString(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& name = get(g.names, g, vertID);
    return name == "string" || name == "u8string";
}

bool SyntaxGraph::isUtf8(vertex_descriptor vertID) const noexcept {
    if (!isString(vertID))
        return false;
    const auto& g = *this;
    const auto& name = get(g.names, g, vertID);
    return name == "u8string";
}

bool SyntaxGraph::isPair(vertex_descriptor vertID, std::pmr::memory_resource* scratch) const noexcept {
    const auto& g = *this;
    auto typePath = get_path(vertID, g, scratch);
    if (typePath.starts_with("/std/pair<") && typePath.ends_with(">")) {
        return true;
    }
    return false;
}

bool SyntaxGraph::isOptional(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    if (!isInstantiation(vertID))
        return false;

    const auto& inst = get<Instance>(vertID, g);
    if (inst.mTemplate == "/std/optional") {
        return true;
    }
    return false;
}

bool SyntaxGraph::isPoolObject(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);
    if (traits.mFlags & POOL_OBJECT)
        return true;

    auto checkStruct = [&](const Composition_ auto& s) {
        bool bPoolObject = false;
        for (const auto& m : s.mMembers) {
            auto memberID = locate(m.mTypePath, g);
            if (memberID == vertID)
                continue;

            if (m.mPointer || m.mReference) {
                Expects(false);
                continue;
            }

            if (g.isPoolObject(memberID)) {
                bPoolObject = true;   
            }
        }
        return bPoolObject;
    };

    return visit_vertex(
        vertID, g,
        [&](const Struct& s) {
            return checkStruct(s);
        },
        [&](const Graph& s) {
            bool bPoolObject = checkStruct(s);
            for (const auto& c : s.mComponents) {
                auto componentID = locate(c.mValuePath, g);
                Expects(componentID != vertID);
                if (g.isPoolObject(componentID))
                    bPoolObject = true;
            }
            for (const auto& c : s.mPolymorphic.mConcepts) {
                auto objectID = locate(c.mValue, g);
                Expects(objectID != vertID);
                if (g.isPoolObject(objectID))
                    bPoolObject = true;
            }
            return bPoolObject;
        },
        [&](const Instance& s) {
            const auto optionalID = locate("/std/optional", g);
            auto templateID = locate(s.mTemplate, g);

            if (s.mTemplate == "/std/shared_ptr"
                || s.mTemplate == "/std/weak_ptr")
                return false;

            if (templateID == optionalID) {
                Expects(s.mParameters.size() == 1);
                auto parameterID = locate(s.mParameters.front(), g);
                return g.isPoolObject(parameterID);
            }

            for (const auto& typePath : s.mParameters) {
                auto paramID = locate(typePath, g);
                if (g.isPoolObject(paramID)) {
                    return true;
                }
            }
            return false;
        },
        [&](const auto&) {
            return false;
        });
}

bool SyntaxGraph::isPoolType(vertex_descriptor vertID, std::string_view typeModulePath) const noexcept {
    const auto& g = *this;
    const auto& modulePath = get(g.modulePaths, g, vertID);
    if (typeModulePath != modulePath) {
        return false;
    }
    if (g.isTypescriptValueType(vertID)) {
        return false;
    }
    const auto parentID = parent(vertID, g);
    if (parentID != SyntaxGraph::null_vertex() && holds_tag<Graph_>(parentID, g)) {
        return false;
    }
    return true;
}

bool SyntaxGraph::isDLL(vertex_descriptor vertID, const ModuleGraph& mg) const noexcept {
    const auto& g = *this;
    const auto& modulePath = get(g.modulePaths, g, vertID);
    const auto& moduleID = locate(modulePath, mg);

    const auto& m = get(mg.modules, mg, moduleID);
    return !m.mAPI.empty();
}

bool SyntaxGraph::isJsb(vertex_descriptor vertID, const ModuleGraph& mg) const noexcept {
    const auto& g = *this;
    auto* scratch = g.mScratch;

    if (g.isValueType(vertID))
        return true;

    const auto& modulePath = get(g.modulePaths, g, vertID);
    const auto& moduleID = locate(modulePath, mg);

    if (moduleID == mg.null_vertex()) {
        const auto& traits = get(g.traits, g, vertID);
        if (traits.mFlags & JSB) {
            return true;
        }
        if (g.isInstantiation(vertID)) {
            auto& inst = get<Instance>(vertID, g);
            auto templateID = locate(inst.mTemplate, g);
            const auto& traits = get(g.traits, g, templateID);
            auto type = g.getTypescriptTypename(templateID, scratch, scratch);
            if (type == "Map" || type == "Array" || type == "Set" || (traits.mFlags & JSB)) {
                for (const auto& param : inst.mParameters) {
                    auto paramID = locate(param, g);
                    if (!g.isJsb(paramID, mg)) {
                        return false;
                    }
                }
                return true;
            }
        }
    } else {
        const auto& m = get(mg.modules, mg, moduleID);

        if (m.mFeatures & ToJs) {
            return true;
        }

        if (!(m.mFeatures & Jsb)) {
            return false;
        }

        const auto& traits = get(g.traits, g, vertID);
        if (traits.mFlags & JSB) {
            return true;
        }
    }

    return false;
}

bool SyntaxGraph::isDerived(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    return !get(g.inherits, g, vertID).mBases.empty();
}

bool SyntaxGraph::hasNoMoveBase(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& bases = get(g.inherits, g, vertID).mBases;
    for (const auto& base : bases) {
        auto baseID = locate(base.mTypePath, g);
        if (g.hasNoMoveBase(baseID)) {
            return true;
        }
        const auto& traits = get(g.traits, g, baseID);
        if (traits.mFlags & NO_MOVE_NO_COPY) {
            return true;
        }
    }
    return false;
}

bool SyntaxGraph::hasNoCopyBase(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& bases = get(g.inherits, g, vertID).mBases;
    for (const auto& base : bases) {
        auto baseID = locate(base.mTypePath, g);
        if (g.hasNoCopyBase(baseID)) {
            return true;
        }
        const auto& traits = get(g.traits, g, baseID);
        if (traits.mFlags & (NO_MOVE_NO_COPY | NO_COPY)) {
            return true;
        }
    }
    return false;
}

bool SyntaxGraph::hasConcreteBase(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& bases = get(g.inherits, g, vertID).mBases;
    for (const auto& base : bases) {
        auto baseID = locate(base.mTypePath, g);
        if (g.hasConcreteBase(baseID)) {
            return true;
        }
        const auto& traits = get(g.traits, g, baseID);
        return !traits.mInterface;
    }
    return false;
}

ImplEnum SyntaxGraph::needDefaultCntr(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);

    bool bNeedDefault = false;
    bool bPmr = g.isPmr(vertID);
    bool bNeedCopy = g.needCopyCntr(vertID) != ImplEnum::None;
    bool bNeedMove = g.needMoveCntr(vertID) != ImplEnum::None;
    bool hasDtor = traits.mFlags & CUSTOM_DTOR;

    if (traits.mInterface)
        bNeedDefault = true;

    if (hasDtor)
        bNeedDefault = true;

    if (traits.mFlags & CUSTOM_CNTR) {
        bNeedDefault = true;
    }

    visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            if (!s.mConstructors.empty())
                bNeedDefault = true;
            if (bPmr)
                bNeedDefault = true;
            if (bNeedCopy)
                bNeedDefault = true;
            if (bNeedMove)
                bNeedDefault = true;
        },
        [&](const auto&) {
        });

    if (traits.mFlags & NO_DEFAULT_CNTR)
        bNeedDefault = false;

    if (bNeedDefault) {
        if (bPmr || traits.mInterface || (traits.mFlags & CUSTOM_CNTR)) {
            return ImplEnum::Separated;
        } else {
            return ImplEnum::Inline;
        }
    } else {
        return ImplEnum::None;
    }
}

ImplEnum SyntaxGraph::needMoveCntr(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);

    bool bPmr = g.isPmr(vertID);
    bool hasDtor = traits.mFlags & CUSTOM_DTOR;
    bool bDerived = g.isDerived(vertID);

    bool bNeedMove = false;
    bool bNeedDeleteMove = false;
    if (traits.mFlags & GenerationFlags::NO_MOVE_NO_COPY) {
        bNeedDeleteMove = true;
    }

    if (hasDtor)
        bNeedMove = true;

    if (bPmr) {
        bNeedMove = true;
    } else if (traits.mFlags & GenerationFlags::NO_COPY) {
        bNeedMove = true;
    }
    
    if (g.hasNoMoveBase(vertID)) {
        return ImplEnum::None;
    }

    if (bNeedDeleteMove) {
        return ImplEnum::Delete;
    } else {
        if (bNeedMove) {
            if (bPmr) {
                return ImplEnum::Separated;
            } else {
                return ImplEnum::Inline;
            }
        } else {
            return ImplEnum::None;
        }
    }
}

ImplEnum SyntaxGraph::needCopyCntr(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);

    bool bPmr = g.isPmr(vertID);
    bool hasDtor = traits.mFlags & CUSTOM_DTOR;
    bool bDerived = !get(g.inherits, g, vertID).mBases.empty();

    bool bNeedCopy = false;
    bool bNeedDeleteCopy = false;

    if (traits.mFlags & GenerationFlags::NO_COPY) {
        bNeedDeleteCopy = true;
    }
    if (traits.mFlags & GenerationFlags::NO_MOVE_NO_COPY) {
        bNeedDeleteCopy = true;
    }

    if (hasDtor)
        bNeedCopy = true;

    if (bPmr) {
        bNeedCopy = true;
    }

    if (g.hasNoCopyBase(vertID)) {
        return ImplEnum::None;
    }

    if (bNeedDeleteCopy) {
        return ImplEnum::Delete;
    } else {
        if (bNeedCopy) {
            if (bPmr) {
                return ImplEnum::Separated;
            } else {
                return ImplEnum::Inline;
            }
        } else {
            return ImplEnum::None;
        }
    }
}

ImplEnum SyntaxGraph::needDtor(vertex_descriptor vertID, bool bDLL) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);

    bool bPmr = g.isPmr(vertID);
    bool hasDtor = traits.mFlags & CUSTOM_DTOR;

    if (hasDtor) {
        return ImplEnum::Separated;
    } else {
        if (traits.mInterface) {
            if (bDLL) {
                return ImplEnum::Separated;
            } else {
                return ImplEnum::Inline;
            }
        }
        if (bDLL && bPmr) {
            return ImplEnum::Separated;
        } else {
            return ImplEnum::None;
        }
    }
}

bool SyntaxGraph::hasPmrOptional(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            for (const Member& m : s.mMembers) {
                auto typeID = locate(m.mTypePath, g);
                if (g.isOptional(typeID)) {
                    const auto& inst = get<Instance>(typeID, g);
                    Expects(inst.mParameters.size() == 1);
                    auto paramID = locate(inst.mParameters.front(), g);
                    if (g.isPmr(paramID)) {
                        return true;
                    }
                }
            }
            return false;
        },
        [&](const auto&) {
            return false;
        });
}

bool SyntaxGraph::hasString(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            for (const Member& m : s.mMembers) {
                auto typeID = locate(m.mTypePath, g);
                if (g.isString(typeID)) {
                    return true;
                }
            }
            return false;
        },
        [&](const auto&) {
            return false;
        });
}

bool SyntaxGraph::hasImpl(vertex_descriptor vertID, bool bDLL) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);

    if (traits.mImport)
        return false;

    if (traits.mUnknown)
        return false;

    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            bool bNeedCopy = g.needCopyCntr(vertID) == ImplEnum::Separated;
            bool bNeedMove = g.needMoveCntr(vertID) == ImplEnum::Separated;
            bool bNeedDefault = g.needDefaultCntr(vertID) == ImplEnum::Separated;
            bool bNeedDtor = g.needDtor(vertID, bDLL) == ImplEnum::Separated;
            bool bPmr = g.isPmr(vertID);

            if (traits.mFlags & CUSTOM_CNTR) {
                bNeedDefault = false;
            }

            return (bPmr && !s.mConstructors.empty())
                || bNeedDefault
                || bNeedMove
                || bNeedCopy
                || bNeedDtor;
        },
        [&](const auto&) {
            return false;
        });
}

bool SyntaxGraph::hasHeader(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);
    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            bool bNeedDefault = g.needDefaultCntr(vertID) != ImplEnum::None;
            bool bNeedMove = g.needMoveCntr(vertID) != ImplEnum::None;
            bool bNeedCopy = g.needCopyCntr(vertID) != ImplEnum::None;
            bool bPmr = g.isPmr(vertID);

            return (bPmr && !s.mConstructors.empty())
                || bNeedDefault || bNeedMove || bNeedCopy;
        },
        [&](const auto&) {
            return true;
        });
}

bool SyntaxGraph::hasVirtualInheritance(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    const auto& traits = get(g.traits, g, vertID);
    const auto& bases = get(g.inherits, g, vertID).mBases;

    bool hasVirtual = false;

    // has special bases
    for (const auto& base : bases) {
        const auto baseID = locate(base.mTypePath, g);
        if (baseID == vertID)
            continue;

        if (base.mVirtualBase) {
            hasVirtual = true;
            break;
        }
        hasVirtual |= g.hasVirtualInheritance(baseID);
    }
    return hasVirtual;
}

bool SyntaxGraph::hasType(vertex_descriptor vertID, vertex_descriptor typeID) const noexcept {
    if (vertID == typeID)
        return true;

    const auto& g = *this;

    if (visit_vertex(
            vertID, g,
            [&](const Struct& s) {
                for (const auto& member : s.mMembers) {
                    const auto& memberID = locate(member.mTypePath, g);
                    if (memberID == vertID)
                        continue;
                    if (hasType(memberID, typeID))
                        return true;
                }
                return false;
            },
            [&](const Graph& s) {
                for (const auto& component : s.mComponents) {
                    const auto& componentID = locate(component.mValuePath, g);
                    if (componentID == vertID)
                        continue;
                    if (hasType(componentID, typeID))
                        return true;
                }
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    const auto& objectID = locate(c.mValue, g);
                    if (objectID == vertID)
                        continue;
                    if (hasType(objectID, typeID))
                        return true;
                }
                for (const auto& member : s.mMembers) {
                    const auto& memberID = locate(member.mTypePath, g);
                    if (memberID == vertID)
                        continue;
                    if (hasType(memberID, typeID))
                        return true;
                }
                return false;
            },
            [&](const Instance& s) {
                for (const auto& paramType : s.mParameters) {
                    const auto& paramID = locate(paramType, g);
                    if (hasType(paramID, typeID))
                        return true;
                }
                return false;
            },
            [&](const auto&) {
                return false;
            })) {
        return true;
    }
    return false;
}

bool SyntaxGraph::hasConsecutiveParameters(
    vertex_descriptor vertID, const Constructor& cntr) const noexcept {
    const auto& g = *this;

    auto prevType = std::make_tuple(g.null_vertex(), false, false);

    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            for (uint32_t i = 0; const auto& m : s.mMembers) {
                bool isParam = std::find(cntr.mIndices.begin(), cntr.mIndices.end(), i) != cntr.mIndices.end();

                if (!isParam)
                    continue;

                auto currType = std::make_tuple(locate(m.mTypePath, g), m.mReference, m.mPointer);
                if (currType == prevType)
                    return true;

                prevType = currType;
            }
            return false;
        },
        [&](const auto&) {
            return false;
        });
}

std::pmr::vector<BaseConstructor> SyntaxGraph::getBaseConstructors(vertex_descriptor vertID) const {
    const auto& g = *this;
    std::pmr::vector<BaseConstructor> results(get_allocator());
    const auto& bases = get(g.inherits, g, vertID).mBases;
    for (const auto& base : bases) {
        const auto baseID = locate(base.mTypePath, g);
        Expects(g.isComposition(baseID));
        BaseConstructor cntr(get_allocator());
        {
            auto baseCntrs = getBaseConstructors(baseID);
            cntr.mBaseID = baseID;
            for (auto& baseCntr : baseCntrs) {
                for (auto& param : baseCntr.mParameters) {
                    cntr.mParameters.emplace_back(std::move(param));
                }
            }
        }
        visit_vertex(
            baseID, g,
            [&](const Composition_ auto& s) {
                if (!s.mConstructors.empty()) {
                    Expects(s.mConstructors.size() == 1);
                    for (const auto& k : s.mConstructors.front().mIndices) {
                        const auto& m = s.mMembers.at(k);
                        Parameter param(get_allocator());
                        param.mTypePath = m.mTypePath;
                        param.mName = getParameterName(m.mMemberName, get_allocator().resource());
                        param.mConst = m.mConst;
                        param.mPointer = m.mPointer;
                        param.mReference = m.mReference;
                        cntr.mParameters.emplace_back(param);
                    }
                }
            },
            [&](const auto&) {
            });

        if (!cntr.mParameters.empty()) {
            results.emplace_back(std::move(cntr));
        }
    }
    return results;
}

SyntaxGraph::vertex_descriptor SyntaxGraph::getMemberType(
    vertex_descriptor vertID, std::string_view member) const noexcept {
    const auto& g = *this;
    Expects(isComposition(vertID));

    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            for (const auto& m : s.mMembers) {
                if (m.mMemberName != member) {
                    continue;
                }
                return locate(m.mTypePath, g);
            }
            return g.null_vertex();
        },
        [&](const auto&) {
            return g.null_vertex();
        });
}

SyntaxGraph::vertex_descriptor SyntaxGraph::getFirstMemberString(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            for (const Member& m : s.mMembers) {
                auto typeID = locate(m.mTypePath, g);
                if (g.isString(typeID) && !g.isUtf8(typeID)) {
                    return typeID;
                }
            }
            return g.null_vertex();
        },
        [&](const auto&) {
            return g.null_vertex();
        });
}

SyntaxGraph::vertex_descriptor SyntaxGraph::getFirstMemberUtf8(vertex_descriptor vertID) const noexcept {
    const auto& g = *this;
    return visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            for (const Member& m : s.mMembers) {
                auto typeID = locate(m.mTypePath, g);
                if (g.isString(typeID) && g.isUtf8(typeID)) {
                    return typeID;
                }
            }
            return g.null_vertex();
        },
        [&](const auto&) {
            return g.null_vertex();
        });
}

namespace {

void collectBasesImpl(const SyntaxGraph& g,
    const std::pmr::vector<Base>& bases,
    std::pmr::set<SyntaxGraph::vertex_descriptor>& baseIDs,
    std::pmr::vector<SyntaxGraph::vertex_descriptor>& results) {
    for (const auto& base : bases) {
        auto baseID = locate(base.mTypePath, g);
        auto& childBases = get(g.inherits, g, baseID).mBases;
        collectBasesImpl(g, childBases, baseIDs, results);
        if (!baseIDs.contains(baseID)) {
            baseIDs.emplace(baseID);
            results.emplace_back(baseID);
        }
    }
}

void collectOverridedImpl(const SyntaxGraph& g,
    const std::pmr::vector<Base>& bases,
    std::pmr::set<SyntaxGraph::vertex_descriptor>& baseIDs,
    std::pmr::vector<SyntaxGraph::vertex_descriptor>& results) {
    for (const auto& base : bases) {
        auto baseID = locate(base.mTypePath, g);
        auto& bases = get(g.inherits, g, baseID).mBases;
        if (!g.isInterface(baseID)) {
            collectBasesImpl(g, bases, baseIDs, results);
            baseIDs.emplace(baseID);
        } else {
            collectOverridedImpl(g, bases, baseIDs, results);
        }
    }
}

}

std::pmr::set<SyntaxGraph::vertex_descriptor>
SyntaxGraph::collectOverrided(vertex_descriptor vertID) const {
    auto scratch = mScratch;
    const auto& g = *this;
    const auto& inherits = get(g.inherits, g, vertID);
    std::pmr::set<SyntaxGraph::vertex_descriptor> bases(scratch);
    std::pmr::vector<SyntaxGraph::vertex_descriptor> results(scratch);
    collectOverridedImpl(g, inherits.mBases, bases, results);
    return bases;
}

bool SyntaxGraph::isPathPmr(const Graph& s) const noexcept {
    Expects(s.mNamed);
    const auto& g = *this;
    if (s.mNamedConcept.mComponent) {
        const auto& c = s.getComponent(s.mNamedConcept.mComponentName);
        const auto componentID = locate(c.mValuePath, g);
        if (s.mNamedConcept.mComponentMemberName.empty()) {
            return g.isPmr(componentID);
        } else {
            return visit_vertex(
                componentID, g,
                [&](const Composition_ auto& s1) {
                    for (const auto& member : s1.mMembers) {
                        if (member.mMemberName == s.mNamedConcept.mComponentMemberName) {
                            auto memberID = locate(member.mTypePath, g);
                            return g.isPmr(memberID);
                        }
                    }
                    // should never reach here
                    Expects(false);
                    return false;
                },
                [&](const auto&) {
                    // should never reach here
                    Expects(false);
                    return false;
                });
        }
    } else {
        auto vpID = locate(s.mVertexProperty, g);
        if (s.mNamedConcept.mComponentMemberName.empty()) {
            return g.isPmr(vpID);
        } else {
            return visit_vertex(
                vpID, g,
                [&](const Composition_ auto& s1) {
                    for (const auto& member : s1.mMembers) {
                        if (member.mMemberName == s.mNamedConcept.mComponentMemberName) {
                            auto memberID = locate(member.mTypePath, g);
                            return g.isPmr(memberID);
                        }
                    }
                    // should never reach here
                    Expects(false);
                    return false;
                },
                [&](const auto&) {
                    // should never reach here
                    Expects(false);
                    return false;
                });
        }
    }
}

std::pmr::string SyntaxGraph::getTypePath(vertex_descriptor vertID,
    std::pmr::memory_resource* mr) const {
    const auto& g = *this;
    Expects(vertID != g.null_vertex());
    auto path = get_path(vertID, g, mr);
    Ensures(!path.empty());
    Ensures(path.front() == '/');
    return path;
}

SyntaxGraph::vertex_descriptor
SyntaxGraph::lookupIdentifier(std::string_view currentScope, std::string_view dependentName,
    std::pmr::memory_resource* scratch) const {
    const auto& g = *this;
    Expects(!dependentName.empty());

    if (dependentName.front() == '/') {
        return locate(dependentName, *this);
    }
    Expects(!dependentName.empty());
    Expects(dependentName.back() != '/');

    // current scope must be root, or is absolute path (for performance sake)
    Expects(currentScope.empty() || currentScope.front() == '/');

    auto validateIdentifier = [](SyntaxGraph::vertex_descriptor vertID, const SyntaxGraph& g) {
        if (vertID == g.null_vertex())
            return;
        visit_vertex(
            vertID, g,
            [&](const Identifier_ auto&) {
                // do nothing
            },
            [&](const auto&) {
                Expects(false);
            });
    };

    auto localScope = currentScope;
    while (!localScope.empty()) {
        // local scope
        Expects(localScope.front() == '/');
        auto parentID = locate(localScope, g);

        // local scope cannot be root
        Ensures(parentID != g.null_vertex());

        // try find identifier in local scope
        auto vertID = locate(parentID, dependentName, g, scratch);
        validateIdentifier(vertID, g);

        if (vertID != g.null_vertex()) {
            // identifier found
            return vertID;
        }

        // move local scope to broader one
        localScope = parentPath(localScope);
    }
    // try find identifier in local scope
    auto vertID = locate(g.null_vertex(), dependentName, g, scratch);
    validateIdentifier(vertID, g);
    return vertID;
}

std::pmr::string SyntaxGraph::getTypePath(
    std::string_view currentScope,
    std::string_view dependentName,
    std::pmr::memory_resource* mr,
    std::pmr::memory_resource* scratch) const {
    if (dependentName.empty()) {
        return std::pmr::string(mr);
    }
    const auto& g = *this;

    if (!isInstance(dependentName)) {
        std::string_view removeCvRefPointer = dependentName;
        bool bConst = false;
        if (removeCvRefPointer.starts_with("const ")) {
            removeCvRefPointer = removeCvRefPointer.substr(6);
            bConst = true;
        }
        bool bVolatile = false;
        if (removeCvRefPointer.starts_with("volatile ")) {
            removeCvRefPointer = removeCvRefPointer.substr(6);
            bVolatile = true;
        }
        Expects(!(bConst && bVolatile));
        bool bReference = false;
        bool bRvalueRenference = false;
        if (removeCvRefPointer.ends_with("&&")) {
            removeCvRefPointer = removeCvRefPointer.substr(0, removeCvRefPointer.size() - 2);
            bRvalueRenference = true;
            bReference = true;
        } else if (removeCvRefPointer.ends_with("&")) {
            removeCvRefPointer = removeCvRefPointer.substr(0, removeCvRefPointer.size() - 1);
            bReference = true;
        }
        bool bPointer = false;
        if (removeCvRefPointer.ends_with("*")) {
            removeCvRefPointer = removeCvRefPointer.substr(0, removeCvRefPointer.size() - 1);
            bPointer = true;
        }
        auto vertID = lookupIdentifier(currentScope, removeCvRefPointer, scratch);
        if (vertID == g.null_vertex()) {
            throw std::out_of_range("identifier not found");
        }
        auto result = getTypePath(vertID, mr);
        if (bVolatile) {
            result.insert(0, "volatile ");
        }
        if (bConst) {
            result.insert(0, "const ");
        }
        if (bPointer) {
            result.append("*");
        }
        if (bRvalueRenference) {
            result.append("&&");
        } else if (bReference) {
            result.append("&");
        }
        return result;
    }

    // is instance
    std::pmr::string result(mr);

    std::pmr::string name(scratch);
    std::pmr::vector<std::pmr::string> parameters(scratch);

    extractTemplate(dependentName, name, parameters);

    std::pmr::string templatePath = getTypePath(currentScope, name, scratch, scratch);
    auto templateID = locate(templatePath, g);
    if (templateID == g.null_vertex()) {
        throw std::out_of_range("template not found");
    }
    Ensures(templateID != g.null_vertex());

    result.append(templatePath);
    result.append("<");

    for (int count = 0; const auto& param : parameters) {
        if (count++)
            result.append(",");
        result.append(getTypePath(currentScope, param, scratch, scratch));
    }
    result.append(">");

    return result;
}

std::pmr::string SyntaxGraph::getDependentName(std::string_view ns, vertex_descriptor vertID,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const {
    if (!isInstanceDependent(vertID)) {
        auto typePath = getTypePath(vertID, mr);
        auto dependentName = getDependentPath(ns, typePath);
        typePath.erase(0, typePath.size() - dependentName.size());
        if (!typePath.empty() && typePath.front() == '/') {
            typePath = typePath.substr(1);
        }
        return typePath;
    }
    const auto& g = *this;
    const auto typePath = getTypePath(vertID, scratch);
    std::pmr::string name(scratch);
    std::pmr::vector<std::pmr::string> parameters(scratch);
    auto suffix = extractTemplate(typePath, name, parameters);

    pmr_ostringstream oss(std::ios_base::out, scratch);

    std::pmr::string templateName(getDependentPath(ns, name), scratch);
    oss << templateName << "<";
    for (int count = 0; const auto& param : parameters) {
        if (count++)
            oss << ", ";
        auto info = extractType(param);
        auto paramID = locate(info.mShortName, g);
        if (info.mConst) {
            oss << "const ";
        }
        oss << getDependentName(ns, paramID, scratch, scratch);
        if (info.mPointer) {
            oss << "*";
        }
    }
    oss << ">";
    oss << suffix;
    auto result = oss.str(std::pmr::polymorphic_allocator<char>(mr));
    if (!result.empty() && result.front() == '/') {
        result = result.substr(1);
    }
    return result;
}

std::pmr::string SyntaxGraph::getDependentCppName(std::string_view ns, vertex_descriptor vertID,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const {
    auto typePath = getDependentName(ns, vertID, scratch, scratch);
    return getCppPath(typePath, mr);
}

SyntaxGraph::vertex_descriptor
SyntaxGraph::lookupType(std::string_view currentScope, std::string_view dependentName,
    std::pmr::memory_resource* scratch) const {
    const auto& g = *this;
    Expects(!dependentName.empty() && dependentName.front() != '/');

    try {
        auto typePath = getTypePath(currentScope, dependentName, scratch, scratch);
        return locate(typePath, g);
    } catch (const std::out_of_range&) {
        // do nothing, type not found
    }

    return g.null_vertex();
}

std::pmr::string SyntaxGraph::getNamespace(vertex_descriptor vertID, std::pmr::memory_resource* mr) const {
    const auto& g = *this;

    // validation
    visit_vertex(
        vertID, g,
        [&](const Identifier_ auto&) {
        },
        [&](const Instantiation_ auto&) {
        },
        [&](const auto&) {
            Expects(false);
        });

    auto parentID = parent(vertID, g);
    while (parentID != g.null_vertex() && !holds_tag<Namespace_>(parentID, g)) {
        parentID = parent(parentID, g);
    }
    if (parentID == g.null_vertex()) {
        return std::pmr::string(mr);
    } else {
        Expects(holds_tag<Namespace_>(parentID, g));
        return g.getTypePath(parentID, mr);
    }
}

std::pmr::string SyntaxGraph::getScope(vertex_descriptor vertID, std::pmr::memory_resource* mr) const {
    const auto& g = *this;

    // validation
    visit_vertex(
        vertID, g,
        [&](const Identifier_ auto&) {
        },
        [&](const Instantiation_ auto&) {
        },
        [&](const auto&) {
            Expects(false);
        });

    return g.getTypePath(vertID, mr);
}

std::pair<std::string_view, std::string_view>
SyntaxGraph::splitTypePath(std::string_view typePath0) const {
    auto typePath = typePath0;
    Expects(typePath.empty() || typePath.front() == '/');
    {
        // remove template parameters
        auto pos = typePath.find_first_of('<');
        typePath = typePath.substr(0, pos);
        // remove identifier
        pos = typePath.find_last_of('/');
        typePath = typePath.substr(0, pos + 1);
    }

    Expects(typePath.empty() || typePath.front() == '/');
    Expects(typePath.empty() || typePath.back() == '/');

    const auto& g = *this;
    size_t posPrev = 0;
    for (auto pos = typePath.find_first_of('/', 1);
        pos != typePath.npos;
        pos = typePath.find_first_of('/', pos + 1)) {
        auto path = typePath.substr(0, pos);
        auto vertID = locate(path, g);
        Expects(vertID != g.null_vertex());
        if (holds_tag<Namespace_>(vertID, g)) {
            posPrev = pos;
        } else {
            Expects(holds_tag<Struct_>(vertID, g) || holds_tag<Graph_>(vertID, g));
            break;
        }
    }
    Ensures(posPrev != typePath.npos);
    Ensures(posPrev < typePath.size());

    return {
        typePath0.substr(0, posPrev), typePath0.substr(posPrev + 1)
    };
}

void SyntaxGraph::instantiate(std::string_view currentScope, std::string_view dependentName,
    std::pmr::memory_resource* scratch) {
    auto& g = *this;

    Expects(isInstance(dependentName));

    std::pmr::string name(scratch);
    std::pmr::vector<std::pmr::string> parameters(scratch);

    extractTemplate(dependentName, name, parameters);

    for (const auto& param : parameters) {
        if (isInstance(param)) {
            instantiate(currentScope, param, scratch);
        }
    }

    auto typePath = getTypePath(currentScope, dependentName, scratch, scratch);

    auto vertID = locate(typePath, g);
    if (vertID == g.null_vertex()) {
        auto [parentNamespace, typeName] = splitTypePath(typePath);
        Expects(isTypePath(parentNamespace));

        auto parentID = locate(parentNamespace, g);
        Expects(parentID != g.null_vertex());

        auto vertID = add_vertex(Instance_{},
            std::forward_as_tuple(typeName), // name
            std::forward_as_tuple(), // trait
            std::forward_as_tuple(), // constraints
            std::forward_as_tuple(), // inherits
            std::forward_as_tuple(), // module path
            std::forward_as_tuple(), // typescript
            std::forward_as_tuple(), // comment
            std::forward_as_tuple(), // polymorphic
            g, parentID);

        extractTemplate(typePath, name, parameters);

        auto& instance = get_by_tag<Instance_>(vertID, g);
        instance.mTemplate = name;
        for (const auto& param : parameters) {
            instance.mParameters.emplace_back(param);
        }
    }
}

void SyntaxGraph::propagate(vertex_descriptor vertID, GenerationFlags flags) {
    auto& g = *this;

    if (g.isValueType(vertID)) {
        return;
    }

    auto& traits = get(g.traits, g, vertID);

    if (flags == GenerationFlags::NO_FLAGS) {
        flags = traits.mFlags;
    }

    GenerationFlags legal = EQUAL | LESS | SPACESHIP | HASH_COMBINE;

    // guard flags
    flags &= static_cast<GenerationFlags>(legal);

    if ((flags & EQUAL) && (traits.mFlags & NO_EQUAL)) {
        flags &= static_cast<GenerationFlags>(~EQUAL);
    }
    if ((flags & LESS) && (traits.mFlags & NO_LESS)) {
        flags &= static_cast<GenerationFlags>(~LESS);
    }
    if ((flags & SPACESHIP) && (traits.mFlags & NO_SPACESHIP)) {
        flags &= static_cast<GenerationFlags>(~SPACESHIP);
    }
    if ((flags & HASH_COMBINE) && (traits.mFlags & NO_HASH_COMBINE)) {
        flags &= static_cast<GenerationFlags>(~HASH_COMBINE);
    }

    traits.mFlags |= flags;

    visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            for (const Member& m : s.mMembers) {
                if (m.mPointer || m.mReference)
                    continue;
                auto memberID = locate(m.mTypePath, g);
                propagate(memberID, flags);
            }
        },
        [&](const Graph& s) {
            for (const auto& c : s.mComponents) {
                auto componentID = locate(c.mValuePath, g);
                propagate(componentID, flags);
            }
            for (const auto& c : s.mPolymorphic.mConcepts) {
                auto conceptID = locate(c.mValue, g);
                propagate(conceptID, flags);
            }
            for (const Member& m : s.mMembers) {
                if (m.mPointer || m.mReference) {
                    continue;
                }
                if (m.mFlags & NOT_ELEMENT) {
                    continue;
                }
                auto memberID = locate(m.mTypePath, g);
                propagate(memberID, flags);
            }
        },
        [&](const Variant& s) {
            if (!g.isTag(vertID)) {
                for (const auto& param : s.mVariants) {
                    auto typeID = locate(param.mTypePath, g);
                    propagate(typeID, flags);
                }
            }
        },
        [&](const Instance& s) {
            for (const auto& p : s.mParameters) {
                auto info = extractType(p);
                if (info.mPointer) {
                    continue;
                }
                auto typeID = locate(p, g);
                propagate(typeID, flags);
            }
        },
        [&](const auto&) {
        });
}

SyntaxGraph::vertex_descriptor SyntaxGraph::getTemplate(
    vertex_descriptor instanceID, std::pmr::memory_resource* scratch) const {
    const auto& g = *this;
    auto typePath = g.getTypePath(instanceID, scratch);
    auto templateName = getTemplateName(typePath);
    auto vertID = locate(templateName, g);
    Ensures(vertID != g.null_vertex());
    return vertID;
}

bool SyntaxGraph::moduleHasMap(std::string_view modulePath, std::string_view mapPath) const {
    const auto& g = *this;
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& path = get(g.modulePaths, g, vertID);
        if (path == modulePath) {
            if (visit_vertex(
                    vertID, g,
                    [&](const Composition_ auto& s) {
                        for (const auto& member : s.mMembers) {
                            const auto& memberID = locate(member.mTypePath, g);
                            if (holds_tag<Instance_>(memberID, g)) {
                                const auto& inst = get<Instance>(memberID, g);
                                if (inst.mTemplate == mapPath)
                                    return true;
                            }
                        }
                        return false;
                    },
                    [&](const auto&) {
                        return false;
                    })) {
                return true;
            }
        }
    }
    return false;
}

bool SyntaxGraph::moduleHasContainer(std::string_view modulePath, std::string_view typePath) const {
    const auto& g = *this;
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& path = get(g.modulePaths, g, vertID);
        if (path == modulePath) {
            if (visit_vertex(
                    vertID, g,
                    [&](const Struct& s) {
                        for (const auto& member : s.mMembers) {
                            const auto& memberID = locate(member.mTypePath, g);
                            if (holds_tag<Instance_>(memberID, g)) {
                                const auto& inst = get<Instance>(memberID, g);
                                if (inst.mTemplate == typePath)
                                    return true;
                            }
                        }
                        return false;
                    },
                    [&](const Graph& s) {
                        for (const auto& component : s.mComponents) {
                            const auto& componentID = locate(component.mValuePath, g);
                            if (holds_tag<Instance_>(componentID, g)) {
                                const auto& inst = get<Instance>(componentID, g);
                                if (inst.mTemplate == typePath)
                                    return true;
                            }
                        }

                        for (const auto& c : s.mPolymorphic.mConcepts) {
                            const auto& objectID = locate(c.mValue, g);
                            if (holds_tag<Instance_>(objectID, g)) {
                                const auto& inst = get<Instance>(objectID, g);
                                if (inst.mTemplate == typePath)
                                    return true;
                            }
                        }

                        for (const auto& member : s.mMembers) {
                            const auto& memberID = locate(member.mTypePath, g);
                            if (holds_tag<Instance_>(memberID, g)) {
                                const auto& inst = get<Instance>(memberID, g);
                                if (inst.mTemplate == typePath)
                                    return true;
                            }
                        }
                        return false;
                    },
                    [&](const auto&) {
                        return false;
                    })) {
                return true;
            }
        }
    }
    return false;
}

bool SyntaxGraph::moduleHasType(std::string_view modulePath, std::string_view typePath) const {
    const auto& g = *this;
    const auto typeID = locate(typePath, g);
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& path = get(g.modulePaths, g, vertID);
        if (path == modulePath) {
            if (hasType(vertID, typeID))
                return true;
        }
    }
    return false;
}

bool SyntaxGraph::moduleHasGraph(std::string_view modulePath) const {
    const auto& g = *this;
    bool hasGraph = false;
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& path = get(g.modulePaths, g, vertID);
        if (path == modulePath) {
            if (holds_tag<Graph_>(vertID, g)) {
                hasGraph = true;
                break;
            }
        }
    }
    return hasGraph;
}

bool SyntaxGraph::moduleHasGraphSerialization(std::string_view modulePath) const {
    const auto& g = *this;
    bool hasGraph = false;
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& path = get(g.modulePaths, g, vertID);
        if (path == modulePath) {
            if (holds_tag<Graph_>(vertID, g)) {
                const auto& traits = get(g.traits, g, vertID);
                if (!(traits.mFlags & NO_SERIALIZATION)) {
                    hasGraph = true;
                    break;
                }
            }
        }
    }
    return hasGraph;
}

bool SyntaxGraph::moduleUsesHashCombine(std::string_view modulePath) const {
    const auto& g = *this;
    bool usesHashCombine = false;
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& path = get(g.modulePaths, g, vertID);
        if (path == modulePath) {
            const auto& traits = get(g.traits, g, vertID);
            if (traits.mFlags & GenerationFlags::HASH_COMBINE) {
                usesHashCombine = true;
                break;
            }
        }
    }
    return usesHashCombine;
}

bool SyntaxGraph::moduleHasImpl(std::string_view modulePath, bool bDLL) const {
    const auto& g = *this;
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& path = get(g.modulePaths, g, vertID);
        if (path != modulePath) {
            continue;
        }
        if (g.hasImpl(vertID, bDLL))
            return true;
    }
    return false;
}

bool SyntaxGraph::isTypescriptValueType(vertex_descriptor vertID) const {
    auto scratch = mScratch;
    const auto& g = *this;
    if (holds_tag<Enum_>(vertID, g))
        return true;

    if (isTag(vertID))
        return true;

    auto tsType = getTypescriptTypename(vertID, scratch, scratch);
    return isTypescriptData(tsType);
}

bool SyntaxGraph::isTypescriptData(std::string_view name) const {
    const auto& g = *this;
    if (name == "number" || name == "string" || name == "boolean") {
        return true;
    }
    return false;
}

bool SyntaxGraph::isTypescriptBoolean(vertex_descriptor vertID) const {
    auto scratch = mScratch;
    const auto& g = *this;
    auto typeName = g.getTypescriptTypename(vertID, scratch, scratch);
    if (typeName == "boolean")
        return true;
    return false;
}

bool SyntaxGraph::isTypescriptNumber(vertex_descriptor vertID) const {
    auto scratch = mScratch;
    const auto& g = *this;
    auto typeName = g.getTypescriptTypename(vertID, scratch, scratch);
    if (typeName == "number")
        return true;
    return false;
}

bool SyntaxGraph::isTypescriptString(vertex_descriptor vertID) const {
    auto scratch = mScratch;
    const auto& g = *this;
    auto typeName = g.getTypescriptTypename(vertID, scratch, scratch);
    if (typeName == "string")
        return true;
    return false;
}

bool SyntaxGraph::isTypescriptArray(vertex_descriptor instanceID,
    std::pmr::memory_resource* scratch) const {
    const auto& g = *this;
    if (!holds_tag<Instance_>(instanceID, g)) {
        return false;
    }
    Expects(holds_tag<Instance_>(instanceID, g));
    const auto& instance = get<Instance>(instanceID, g);

    auto templateID = g.getTemplate(instanceID, scratch);
    const auto& templateTS = get(g.typescripts, g, templateID);

    // 1. is container
    // 2. has only one parameter
    // 3. not specialized
    if (holds_tag<Container_>(templateID, g)
        && instance.mParameters.size() == 1
        && templateTS.mName.empty()) {
        return true;
    }

    return false;
}

bool SyntaxGraph::isTypescriptTypedArray(vertex_descriptor vertID) const {
    static const Set<std::string> types = {
        "Int8Array",
        "Int16Array",
        "Int32Array",
        "BigInt64Array",
        "Uint8Array",
        "Uint16Array",
        "Uint32Array",
        "BigUint64Array",
    };

    auto scratch = mScratch;
    const auto& g = *this;
    if (!holds_tag<Instance_>(vertID, g)) {
        return false;
    }
    Expects(holds_tag<Instance_>(vertID, g));
    const auto& name = getTypescriptTypename(vertID, scratch, scratch);
    if (name.empty()) {
        return false;
    }
    if (types.contains(std::string_view(name))) {
        return true;
    }
    return false;
}

bool SyntaxGraph::isTypescriptSet(vertex_descriptor vertID) const {
    auto scratch = mScratch;
    const auto& g = *this;
    if (!holds_tag<Instance_>(vertID, g)) {
        return false;
    }
    Expects(holds_tag<Instance_>(vertID, g));
    const auto& instance = get<Instance>(vertID, g);

    auto templateID = g.getTemplate(vertID, scratch);
    const auto& templateTS = get(g.typescripts, g, templateID);

    // 1. is container
    // 2. has only one parameter
    // 3. not specialized
    if (holds_tag<Container_>(templateID, g)
        && instance.mParameters.size() == 1
        && templateTS.mName == "Set") {
        return true;
    }

    return false;
}

bool SyntaxGraph::isTypescriptMap(vertex_descriptor vertID) const {
    auto scratch = mScratch;
    const auto& g = *this;
    if (!holds_tag<Instance_>(vertID, g)) {
        return false;
    }
    Expects(holds_tag<Instance_>(vertID, g));
    const auto& instance = get<Instance>(vertID, g);

    auto templateID = g.getTemplate(vertID, scratch);
    if (holds_tag<Map_>(templateID, g)) {
        return true;
    }
    return false;
}

bool SyntaxGraph::isTypescriptPointer(vertex_descriptor vertID) const {
    const auto& g = *this;
    auto scratch = mScratch;
    if (holds_tag<Instance_>(vertID, g)) {
        if (!g.isTypescriptArray(vertID, scratch)) {
            auto templateID = g.getTemplate(vertID, scratch);
            const auto& templateTS = get(g.typescripts, g, templateID);
            if (templateTS.mName == "_") {
                return true;
            }
        }
    }
    return false;
}

std::pmr::string SyntaxGraph::getTypescriptTypename(vertex_descriptor vertID,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const {
    const auto& g = *this;
    
    if (vertID == g.null_vertex()) {
        return std::pmr::string(mr);
    }

    const auto& name = get(g.names, g, vertID);
    const auto& ts = get(g.typescripts, g, vertID);

    std::pmr::string result(mr);

    visit_vertex(
        vertID, g,
        [&](const Identifier_ auto& v) {
            if (ts.mName.empty()) {
                result = name;
            } else {
                result = ts.mName;
            }
        },
        [&](const Instance& instance) {
            if (!ts.mName.empty()) {
                // template full specialization
                result = ts.mName;
                return;
            }

            auto templateID = g.getTemplate(vertID, scratch);
            const auto& templateName = get(g.names, g, templateID);
            const auto& templateTS = get(g.typescripts, g, templateID);

            if (g.isTypescriptArray(vertID, scratch)) {
                Expects(instance.mParameters.size() == 1);
                const auto& param = instance.mParameters.front();
                const auto paramPath = removeCvPointerRef(param);
                auto paramID = locate(paramPath, g);
                Expects(paramID != g.null_vertex());
                auto paramName = g.getTypescriptTypename(paramID, scratch, scratch);
                Expects(!paramName.empty());
                result.append(paramName);
                result.append("[]");   
            } else {
                if (templateTS.mName == "_") {
                    // is removed
                    Expects(instance.mParameters.size() == 1);
                    const auto& param = instance.mParameters.front();
                    auto paramID = locate(param, g);
                    auto paramName = g.getTypescriptTypename(paramID, scratch, scratch);
                    result.append(paramName);
                } else {
                    // is template
                    if (templateTS.mName.empty()) {
                        result.append(templateName);
                    } else {
                        result.append(templateTS.mName);
                    }
                    result.append("<");
                    int count = 0;
                    for (const auto& param : instance.mParameters) {
                        const auto paramTraits = getParameterTraits(param);
                        const auto paramTypePath = removeCvPointerRef(param);
                        auto paramID = locate(paramTypePath, g);
                        auto paramName = g.getTypescriptTypename(paramID, scratch, scratch);
                        if (count++) {
                            result.append(", ");
                        } else {
                            const auto& paramTraits = get(g.traits, g, paramID);
                            if (paramTraits.mFlags & STRING_KEY) {
                                paramName = "string";
                            }
                        }
                        result.append(paramName);
                    }
                    result.append(">");
                }
            }
        },
        [&](const Concept&) {
            Expects(false);
        },
        [&](const Define&) {
            Expects(false);
        });

    Ensures(!result.empty());
    Ensures(!boost::algorithm::contains(result, "/"));
    return result;
}

std::pmr::string SyntaxGraph::getTypescriptTypename(std::string_view typePath,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const {
    auto vertID = locate(typePath, *this);
    return getTypescriptTypename(vertID, mr, scratch);
}

std::pmr::string SyntaxGraph::getTypescriptTagName(vertex_descriptor vertID,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const {
    auto name = getTypescriptTypename(vertID, mr, scratch);
    if (!name.empty() && name.back() == '_') {
        name.pop_back();
    }
    return name;
}

std::pmr::string SyntaxGraph::getTypescriptTagName(std::string_view typePath,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const {
    auto vertID = locate(typePath, *this);
    return getTypescriptTagName(vertID, mr, scratch);
}

std::pmr::string SyntaxGraph::getTypescriptInitialValue(
    vertex_descriptor vertID,
    std::string_view tsDefaultValue,
    std::string_view cppDefautValue, bool bPointer) const {
    auto* scratch = mScratch;
    const auto& g = *this;
    const auto& ts = get(g.typescripts, g, vertID);
    const auto& traits = get(g.traits, g, vertID);

    if (!tsDefaultValue.empty())
        return std::pmr::string(tsDefaultValue, scratch);

    Expects(cppDefautValue != "_");
    std::pmr::string initial1(cppDefautValue, scratch);

    boost::algorithm::trim(initial1);
    if (!initial1.empty() && initial1.front() == '{' && initial1.back() == '}') {
        Expects(initial1.size() >= 2);
        initial1 = initial1.substr(1, initial1.size() - 2);
        boost::algorithm::trim(initial1);
    }
    boost::algorithm::replace_all(initial1, "::", ".");
    boost::algorithm::replace_all(initial1, "/", ".");

    {
        auto pos = initial1.rfind('.');
        if (pos != initial1.npos) {
            Expects(pos != 0);
            pos = initial1.rfind('.', pos - 1);
            if (pos != initial1.npos) {
                initial1 = initial1.substr(pos + 1);
            }
        }
    }

    std::string_view initial = initial1;

    pmr_ostringstream oss(std::ios_base::out, scratch);

    auto generateInitialValue = [&]() {
        Expects(!initial.empty());
        if (ts.mName == "number" || ts.mName == "BigInt") {
            if (initial.back() == 'u') {
                initial = initial.substr(0, initial.size() - 1);
            } else if (initial.back() == 'f') {
                initial = initial.substr(0, initial.size() - 1);
            }
            if (ts.mName == "BigInt") {
                oss << "BigInt(" << initial << ")";
            } else {
                oss << initial;
            }
        } else if (ts.mName == "string") {
            std::pmr::string str(scratch);
            if (initial.substr(0, 2) == "u8") {
                str = initial.substr(2);
            } else {
                str = initial;
            }
            boost::algorithm::replace_all(str, "\"", "'");
            oss << str;
        } else if (ts.mName == "boolean") {
            oss << initial;
        } else if (ts.mName == "Int8Array") {
            oss << "new Int8Array(" << initial << ")";
        } else if (ts.mName == "Int16Array") {
            oss << "new Int16Array(" << initial << ")";
        } else if (ts.mName == "Int32Array") {
            oss << "new Int32Array(" << initial << ")";
        } else if (ts.mName == "BigInt64Array") {
            oss << "new BigInt64Array(" << initial << ")";
        } else if (ts.mName == "Uint8Array") {
            oss << "new Uint8Array(" << initial << ")";
        } else if (ts.mName == "Uint16Array") {
            oss << "new Uint16Array(" << initial << ")";
        } else if (ts.mName == "Uint32Array") {
            oss << "new Uint32Array(" << initial << ")";
        } else if (ts.mName == "BigUint64Array") {
            oss << "new BigUint64Array(" << initial << ")";
        } else {
            if (traits.mFlags & POOL_OBJECT) {
                oss << g.getTypescriptTypename(vertID, scratch, scratch)
                    << ".create()";
            } else {
                oss << "new ";
                oss << g.getTypescriptTypename(vertID, scratch, scratch);
                oss << "(" << initial << ")";
            }
        }
    };

    auto generateDefaultTsValue = [&]() {
        if (ts.mName == "number" || ts.mName == "BigInt") {
            oss << "0";
        } else if (ts.mName == "string") {
            oss << "\'\'";
        } else if (ts.mName == "boolean") {
            oss << "false";
        } else if (ts.mName == "Int8Array") {
            oss << "new Int8Array(0)";
        } else if (ts.mName == "Int16Array") {
            oss << "new Int16Array(0)";
        } else if (ts.mName == "Int32Array") {
            oss << "new Int32Array(0)";
        } else if (ts.mName == "BigInt64Array") {
            oss << "new BigInt64Array(0)";
        } else if (ts.mName == "Uint8Array") {
            oss << "new Uint8Array(0)";
        } else if (ts.mName == "Uint16Array") {
            oss << "new Uint16Array(0)";
        } else if (ts.mName == "Uint32Array") {
            oss << "new Uint32Array(0)";
        } else if (ts.mName == "BigUint64Array") {
            oss << "new BigUint64Array(0)";
        } else {
            oss << "new ";
            oss << g.getTypescriptTypename(vertID, scratch, scratch);
            oss << "()";
        }
    };

    auto outputNull = [&]() {
        if (initial.empty()) {
            oss << "null";
        } else {
            if (initial == "nullptr"
                || initial == "NULL"
                || initial == "0") {
                oss << "null";
            } else {
                oss << initial;
            }
        }
    };
    if (bPointer || g.isTypescriptPointer(vertID)) {
        outputNull();
        return oss.str();
    }

    visit_vertex(
        vertID, g,
        [&](const Enum& e) {
            if (!initial.empty()) {
                oss << initial;
            } else {
                Expects(!e.mValues.empty());
                oss << g.getTypescriptTypename(vertID, scratch, scratch)
                    << "." << e.mValues.front().mName;
            }
        },
        [&](const Variant& v) {
            if (!initial.empty()) {
                oss << initial;
            } else {
                Expects(!v.mVariants.empty());
                oss << g.getTypescriptTypename(vertID, scratch, scratch)
                    << "." << g.getTypescriptTagName(v.mVariants.front().mTypePath, scratch, scratch);
            }
        },
        [&](const Identifier_ auto& v) {
            if (!initial.empty()) {
                generateInitialValue();
            } else {
                generateDefaultTsValue();
            }
        },
        [&](const Instance& instance) {
            if (!ts.mName.empty()) {
                if (!initial.empty()) {
                    generateInitialValue();
                } else {
                    generateDefaultTsValue();
                }
                return;
            }
            if (g.isTypescriptArray(vertID, scratch)) {
                oss << "[" << initial << "]";
            } else {
                auto templateID = g.getTemplate(vertID, scratch);
                const auto& templateName = get(g.names, g, templateID);
                const auto& templateTS = get(g.typescripts, g, templateID);
                auto name = g.getTypescriptTypename(vertID, scratch, scratch);
                oss << "new " << name << "(" << initial << ")";
            }
        },
        [&](const Concept&) {
            Expects(false);
        },
        [&](const Define&) {
            Expects(false);
        });

    auto result = oss.str();
    Ensures(!result.empty());
    Ensures(!boost::algorithm::contains(result, "/"));
    return result;
}

std::pmr::string SyntaxGraph::getTypescriptInitialValue(
    vertex_descriptor vertID, const Member& m,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const {
    return getTypescriptInitialValue(vertID, m.mTypescriptDefaultValue, m.mDefaultValue, m.mPointer);
}

std::pmr::string SyntaxGraph::getTypescriptGraphPolymorphicVariant(const Graph& s,
    std::pmr::memory_resource* mr,
    std::pmr::memory_resource* scratch) const {
    const auto& g = *this;
    pmr_ostringstream oss(std::ios_base::out, mr);
    int count = 0;
    for (const auto& c : s.mPolymorphic.mConcepts) {
        if (count++) {
            oss << " | ";
        }
        oss << g.getTypescriptTypename(c.mValue, scratch, scratch);
    }
    return oss.str();
}

std::pmr::string SyntaxGraph::getTypedParameterName(
    const Parameter& p, bool bPublic, bool bFull, bool bOptional, bool bReturn) const {
    const auto& g = *this;
    auto scratch = mScratch;

    auto memberID = locate(p.mTypePath, g);
    auto typeName = g.getTypescriptTypename(memberID, scratch, scratch);
    Expects(!typeName.empty());

    std::pmr::string result(scratch);

    if (bFull || !g.isTypescriptData(typeName)) {
        if (p.mTypePath == "/void" && p.mPointer) {
            result += "unknown";
        } else {
            result += typeName;
            if (p.mNullable) {
                result += " | null";
            }
            if (bOptional && bReturn) {
                // currently, reference is not supported
                // we must use pointer
                result += " | undefined";
            }
        }
    }

    return result;
}

namespace {

void addImported(SyntaxGraph::vertex_descriptor vertID, const SyntaxGraph& g,
    std::string_view modulePath,
    PmrMap<std::pmr::string, PmrSet<std::pmr::string>>& imported) {

    const auto& path = get(g.modulePaths, g, vertID);
    if (!path.empty() && path != modulePath) {
        imported[path].emplace(g.getTypePath(vertID, imported.get_allocator().resource()));
    }

    visit_vertex(
        vertID, g,
        [&](const Composition_ auto& s) {
            if (false) { // non-recursive
                for (const Member& m : s.mMembers) {
                    auto memberID = locate(m.mTypePath, g);
                    addImported(memberID, g, modulePath, imported);
                }
                for (const Method& m : s.mMethods) {
                    for (const auto& param : m.mParameters) {
                        auto paramID = locate(param.mTypePath, g);
                        addImported(paramID, g, modulePath, imported);
                    }
                    auto paramID = locate(m.mReturnType.mTypePath, g);
                    addImported(paramID, g, modulePath, imported);
                }
            }
        },
        [&](const Instance& s) {
            for (const auto& p : s.mParameters) {
                auto typePath = removeCvPointerRef(p);
                auto paramID = locate(typePath, g);
                addImported(paramID, g, modulePath, imported);
            }
        },
        [](const auto&) {});
}

}

PmrMap<std::pmr::string, PmrSet<std::pmr::string>> SyntaxGraph::getImportedTypes(
    std::string_view modulePath, std::pmr::memory_resource* mr) const {
    PmrMap<std::pmr::string, PmrSet<std::pmr::string>> imported(mr);

    const auto& g = *this;
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& path = get(g.modulePaths, g, vertID);
        if (path != modulePath)
            continue;

        visit_vertex(vertID, g,
            [&](const Composition_ auto& s) {
                for (const Member& m : s.mMembers) {
                    auto memberID = locate(m.mTypePath, g);
                    addImported(memberID, g, modulePath, imported);
                }
                for (const Method& m : s.mMethods) {
                    for (const auto& param : m.mParameters) {
                        auto paramID = locate(param.mTypePath, g);
                        addImported(paramID, g, modulePath, imported);
                    }
                    auto paramID = locate(m.mReturnType.mTypePath, g);
                    addImported(paramID, g, modulePath, imported);
                }
            },
            [](const auto&) {});

        visit_vertex(
            vertID, g,
            [&](const Graph& s) {
                if (!s.mVertexProperty.empty()) {
                    auto typeID = locate(s.mVertexProperty, g);
                    addImported(typeID, g, modulePath, imported);
                }
                if (!s.mEdgeProperty.empty()) {
                    auto typeID = locate(s.mEdgeProperty, g);
                    addImported(typeID, g, modulePath, imported);
                }
                for (const auto& c : s.mComponents) {
                    auto typeID = locate(c.mValuePath, g);
                    addImported(typeID, g, modulePath, imported);
                }
                for (const auto& c : s.mPolymorphic.mConcepts) {
                    auto typeID = locate(c.mValue, g);
                    addImported(typeID, g, modulePath, imported);
                }
            },
            [](const auto&) {});
    }
    return imported;
}

std::pmr::string Graph::getTypescriptVertexDescriptorType(
    std::string_view tsName, std::pmr::memory_resource* scratch) const {
    return visit(
        overload(
            [&](Vector_) {
                return std::pmr::string("number", scratch);
            },
            [&](List_) {
                return std::pmr::string(tsName, scratch) + "Vertex";
            }),
        mVertexListType);
}

std::string_view Graph::getTypescriptEdgeDescriptorType() const {
    if (mEdgeProperty.empty()) {
        if (gImpl) {
            return "impl.ED";
        } else {
            return "ED";
        }
    } else {
        if (gImpl) {
            return "impl.EPD";
        } else {
            return "EPD";
        }
    }
}

std::string_view Graph::getTypescriptReferenceDescriptorType() const {
    if (isAliasGraph()) {
        return getTypescriptEdgeDescriptorType();
    } else {
        if (gImpl) {
            return "impl.ED";
        } else {
            return "ED";
        }
    }
}

std::pmr::string Graph::getTypescriptVertexDereference(std::string_view v,
    std::pmr::memory_resource* scratch) const {
    pmr_ostringstream oss(std::ios_base::out, scratch);

    if (isVector()) {
        oss << "this." << gNameVertices << "[" << v << "]";
    } else {
        oss << v;
    }

    return oss.str();
}

std::string_view Graph::getTypescriptOutEdgeList(bool bAddressable) const {
    if (bAddressable) {
        if (mAliasGraph) {
            return gNameOutEdgeList;
        } else {
            return gNameChildrenList;
        }
    } else {
        return gNameOutEdgeList;
    }
}

std::string_view Graph::getTypescriptInEdgeList(bool bAddressable) const {
    if (bAddressable) {
        return gNameParentsList;
    } else {
        return gNameInEdgeList;
    }
}

std::pmr::string Component::getTypescriptComponentType(const SyntaxGraph& g,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const noexcept {
    auto vertID = locate(mValuePath, g);
    return g.getTypescriptTypename(vertID, mr, scratch);
}

std::string_view Graph::getTypescriptNullVertex() const {
    return visit(
        overload(
            [&](Vector_) {
                return std::string_view("0xFFFFFFFF");
            },
            [&](List_) {
                return std::string_view("null");
            }),
        mVertexListType);
}

std::pmr::string Graph::getTypescriptVertexPropertyType(const SyntaxGraph& g,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const noexcept {
    auto vertID = locate(mVertexProperty, g);
    return g.getTypescriptTypename(vertID, mr, scratch);
}

}
