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

#include "SyntaxTypes.h"
#include "SyntaxGraphs.h"
#include "SyntaxUtils.h"

namespace Cocos::Meta {

std::pmr::string Graph2::getTypescriptVertexDescriptorType(
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

std::string_view Graph2::getTypescriptEdgeDescriptorType() const {
    if (mEdgeProperty.empty()) {
        return "impl.ED";
    } else {
        return "impl.EPD";
    }
}

std::string_view Graph2::getTypescriptReferenceDescriptorType() const {
    if (isAliasGraph()) {
        return getTypescriptEdgeDescriptorType();
    } else {
        return "impl.ED";
    }
}

std::pmr::string Graph2::getTypescriptVertexDereference(std::string_view v,
    std::pmr::memory_resource* scratch) const {
    pmr_ostringstream oss(std::ios_base::out, scratch);

    if (isVector()) {
        oss << "this._vertices[" << v << "]";
    } else {
        oss << v;
    }

    return oss.str();
}

std::string_view Graph2::getTypescriptOutEdgeList(bool bAddressable) const {
    if (bAddressable) {
        if (mAliasGraph) {
            return "_outEdges";
        } else {
            return "_children";
        }
    } else {
        return "_outEdges";
    }
}

std::string_view Graph2::getTypescriptInEdgeList(bool bAddressable) const {
    if (bAddressable) {
        return "_parents";
    } else {
        return "_inEdges";
    }
}

std::pmr::string Component::getTypescriptComponentType(const SyntaxGraph& g,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const noexcept {
    auto vertID = locate(mValuePath, g);
    return g.getTypescriptTypename(vertID, mr, scratch);
}

std::string_view Graph2::getTypescriptNullVertex() const {
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

std::pmr::string Graph2::getTypescriptVertexPropertyType(const SyntaxGraph& g,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const noexcept {
    auto vertID = locate(mVertexProperty, g);
    return g.getTypescriptTypename(vertID, mr, scratch);
}

bool SyntaxGraph::isNamespace(std::string_view typePath) const noexcept {
    const auto& g = *this;
    if (typePath.empty()) {
        return true;
    }
    auto parentID = locate(typePath, g);
    return parentID != g.null_vertex();
}

bool SyntaxGraph::isTag(vertex_descriptor vertID) const {
    const auto& g = *this;
    if (holds_tag<Tag_>(vertID, g))
        return true;

    if (holds_tag<Variant_>(vertID, g)) {
        const auto& var = get_by_tag<Variant_>(vertID, g);
        for (const auto& typePath : var.mVariants) {
            auto typeID = locate(typePath, g);
            if (!holds_tag<Tag_>(typeID, g)) {
                return false;
            }
        }
        return true;
    }
    return false;
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
        auto vertID = lookupIdentifier(currentScope, dependentName, scratch);
        if (vertID == g.null_vertex()) {
            throw std::out_of_range("identifier not found");
        }
        return getTypePath(vertID, mr);
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
            std::forward_as_tuple(), // module path
            std::forward_as_tuple(), // typescript
            std::forward_as_tuple(), // polymorphic
            g, parentID);

        extractTemplate(typePath, name, parameters);

        auto& instance = get_by_tag<Instance_>(vertID, g);
        for (const auto& param : parameters) {
            instance.mParameters.emplace_back(param);
        }
    }
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

bool SyntaxGraph::isTypescriptData(std::string_view name) const {
    const auto& g = *this;
    if (name == "number" || name == "string" || name == "boolean") {
        return true;
    }
    return false;
}

bool SyntaxGraph::isTypescriptArray(vertex_descriptor instanceID,
    std::pmr::memory_resource* scratch) const {
    const auto& g = *this;
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
                auto paramID = locate(param, g);
                auto paramName = g.getTypescriptTypename(paramID, scratch, scratch);
                result.append(paramName);
                result.append("[]");   
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
                    auto paramID = locate(param, g);
                    auto paramName = g.getTypescriptTypename(paramID, scratch, scratch);
                    if (count++) {
                        result.append(", ");
                    }
                    result.append(paramName);
                }
                result.append(">");
            }
        },
        [&](const Concept&) {
            Expects(false);
        },
        [&](const Declare&) {
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
    vertex_descriptor vertID, std::string_view initial0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) const {
    const auto& g = *this;
    const auto& ts = get(g.typescripts, g, vertID);

    Expects(initial0 != "_");
    std::pmr::string initial1(initial0, scratch);

    boost::algorithm::trim(initial1);
    if (!initial1.empty() && initial1.front() == '{' && initial1.back() == '}') {
        Expects(initial1.size() >= 2);
        initial1 = initial1.substr(1, initial1.size() - 2);
        boost::algorithm::trim(initial1);
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
            oss << "new ";
            oss << g.getTypescriptTypename(vertID, scratch, scratch);
            oss << "(" << initial << ")";
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
                    << "." << g.getTypescriptTagName(v.mVariants.front(), scratch, scratch);
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
                oss << "new ";
                if (templateTS.mName.empty()) {
                    oss << templateName;
                } else {
                    oss << templateTS.mName;
                }
                oss << "<";
                int count = 0;
                for (const auto& param : instance.mParameters) {
                    auto paramID = locate(param, g);
                    auto paramName = g.getTypescriptTypename(paramID, scratch, scratch);
                    if (count++) {
                        oss << ", ";
                    }
                    oss << paramName;
                }
                oss << ">(" << initial << ")";
            }
        },
        [&](const Concept&) {
            Expects(false);
        },
        [&](const Declare&) {
            Expects(false);
        });

    auto result = oss.str();
    Ensures(!result.empty());
    Ensures(!boost::algorithm::contains(result, "/"));
    return result;
}

std::pmr::string SyntaxGraph::getTypescriptGraphPolymorphicVariant(const Graph2& s,
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

bool SyntaxGraph::moduleUsesGraph(std::string_view modulePath) const {
    const auto& g = *this;
    bool usesGraph = false;
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& path = get(g.modulePaths, g, vertID);
        if (path == modulePath) {
            if (holds_tag<Graph_>(vertID, g)) {
                usesGraph = true;
                break;
            }
        }
    }
    return usesGraph;
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
            for (const Member& m : s.mMembers) {
                auto memberID = locate(m.mTypePath, g);
                addImported(memberID, g, modulePath, imported);
            }
        },
        [&](const Instance& s) {
            for (const auto& p : s.mParameters) {
                auto paramID = locate(p, g);
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
            },
            [](const auto&) {});

        visit_vertex(
            vertID, g,
            [&](const Graph2& s) {
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

}
