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

#include "CppBuilder.h"
#include "SyntaxGraphs.h"
#include "SyntaxUtils.h"

namespace Cocos::Meta {

namespace {

bool outputNamespaces(std::ostream& oss, std::pmr::string& space,
    CodegenContext& context, std::string_view typePath) {
    bool bNewNamespace = false;
    size_t end = 0;
    auto pos = typePath.find_first_of('/');
    for (size_t i = 0; i != context.mNamespaces.size(); ++i) {
        if (pos == std::string_view::npos) {
            break;
        }
        auto pos1 = typePath.find_first_of('/', pos + 1);
        auto ns = typePath.substr(pos + 1, pos1 - (pos + 1));
        if (ns != context.mNamespaces[i]) {
            break;
        }

        if (pos1 != std::string_view::npos) {
            pos = pos1;
        } else {
            pos = std::string_view::npos;
        }
        ++end;
    }

    for (auto i = context.mNamespaces.size(); i-- > end;) {
        oss << "\n";
        OSS << "} // " << context.mNamespaces[i] << "\n";
        context.mNamespaces.pop_back();
    }

    while (pos != std::string_view::npos) {
        auto pos1 = typePath.find_first_of('/', pos + 1);
        auto name = typePath.substr(pos + 1, pos1 - (pos + 1));
        oss << "\n";
        OSS << "namespace " << name << " {\n";
        context.mNamespaces.emplace_back(name);

        if (pos1 != std::string_view::npos) {
            pos = pos1;
        } else {
            pos = std::string_view::npos;
        }
        bNewNamespace = true;
    }

    return bNewNamespace;
}

}

std::pmr::string generateFwd_h(const SyntaxGraph& g, std::string_view moduleName0,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);
    CodegenContext context(scratch);
    
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& moduleName = get(g.modulePaths, g, vertID);
        if (moduleName != moduleName0) {
            continue;
        }

        auto typePath = g.getTypePath(vertID, scratch);
        auto ns = g.getNamespace(vertID, scratch);

        auto bNewNamespace = outputNamespaces(oss, space, context, ns);
        if (bNewNamespace) {
            oss << "\n";
        }

        const auto& name = get(g.names, g, vertID);
        const auto& t = get(g.traits, g, vertID);

        visit_vertex(
            vertID, g,
            [&](const Enum& e) {
                if (!e.mUnderlyingType.empty() || t.mClass) {
                    OSS << "enum ";
                    if (t.mClass)
                        oss << "class ";
                    oss << name;
                    if (!e.mUnderlyingType.empty()) {
                        oss << " : " << e.mUnderlyingType;
                    }
                    oss << ";\n";
                }
            },
            [&](const Tag& t) {
                OSS << "struct " << name << ";\n";
            },
            [&](const Variant& v) {
                OSS << "using " << name << " = std::variant<";
                for (auto count = 0; const auto& e : v.mVariants) {
                    if (count++)
                        oss << ", ";
                    OSS << getCppPath(getDependentPath(ns, e), scratch);
                }
                oss << ">;\n";
            },
            [&](const Composition_ auto&) {
                OSS << "struct " << name << ";\n";
            },
            [&](const auto&) {
            });
    }

    outputNamespaces(oss, space, context, "");

	return oss.str();
}

std::pmr::string generateTypes_h(const SyntaxGraph& g, std::string_view moduleName,
    std::pmr::memory_resource* mr, std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss(std::ios_base::out, mr);
    std::pmr::string space(scratch);

    return oss.str();
}

}
