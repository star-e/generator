#include "WasmBuilder.h"
#include "BuilderTypes.h"
#include "SyntaxGraphs.h"
#include "SyntaxUtils.h"

namespace Cocos::Meta {

namespace {

void outputMemberFunction(){

};

template <typename T>
concept TypeAddressableObject = requires {
    T::mPointer;
    T::mTypePath;
};

template <TypeAddressableObject T>
void registerPointer(const T& member, std::pmr::unordered_set<std::pmr::string>& registerPtrs, const SyntaxGraph& g, std::pmr::memory_resource* scratch) {
    if (member.mPointer) {
        const auto& pathID = locate(member.mTypePath, g);
        const auto& typeName = g.getDependentCppName("", pathID, scratch, scratch);
        registerPtrs.emplace(typeName);
    }
}

void outputStruct(std::ostream& oss, std::pmr::unordered_set<std::pmr::string>& registerPtrs, std::string_view structCppName,
    std::string_view name, const std::pmr::vector<Member>& members,
    const SyntaxGraph& g, std::pmr::memory_resource* scratch) {
    oss << "\n";
    oss << "value_object<" << structCppName << R"(>(")" << name << R"("))";
    for (const auto& member : members) {
        registerPointer(member, registerPtrs, g, scratch);
        oss << "\n";
        oss << "\t"
            << R"(.field(")" << member.getMemberName() << R"(", &)"
            << structCppName << "::" << member.getMemberName() << ")";
    }
    oss << ";";
};

void defineGetter(std::ostream& forwardDefs,
    const std::pmr::string& className,
    const std::pmr::string& name,
    const std::string_view prop,
    std::pmr::memory_resource* scratch) {
    forwardDefs << "\nconst auto & " << name << "_" << prop << "_getter(const " << className << "& v) {\n"
                << "\treturn v." << prop << ";\n}\n";
};

void defineSetter(std::ostream& forwardDefs,
    const std::pmr::string& className,
    const std::pmr::string& name,
    const Member& member,
    const SyntaxGraph& g,
    std::pmr::memory_resource* scratch) {
    const auto& memberTypePath = member.mTypePath;
    const auto& prop = member.getMemberName();

    auto pathID = locate(memberTypePath, g);
    const auto& memberTypename = g.getDependentCppName("", pathID, scratch, scratch);

    std::string_view instName = memberTypePath;
    while (instName.ends_with('>')) {
        instName = peel(instName);
    }

    auto idx = instName.find_first_of(',');
    if (instName.find_first_of(',') != std::string::npos) {
        instName = instName.substr(idx + 1);
    }

    forwardDefs << std::vformat(R"(
void {0}_{1}_setter({2}& t, const {3}& v) {{
    assignVal(t.{1}, v);
}};
)",
        std::make_format_args(name, prop, className, memberTypename));
};

void outputClassProperty(std::ostream& oss,
    const std::pmr::string& className,
    const std::pmr::string& dependentName,
    const std::string_view prop,
    const std::string_view getter,
    const std::string_view setter,
    std::pmr::memory_resource* scratch) {

    Expects(!getter.empty());

    oss << "\n";
    oss << "\t"
        << R"(.property(")" << prop << R"(", &)" << getter;
    if (!setter.empty()) {
        oss << R"(, &)" << setter;
    }
    oss << ")";
}

void oututClassFunction(std::ostream& oss,
    std::pmr::unordered_set<std::pmr::string>& registerPtrs,
    const std::pmr::string& dependentCppName,
    const Method& method,
    const SyntaxGraph& g,
    std::pmr::memory_resource* scratch) {
    const auto& funcName = method.mFunctionName;
    Expects(!funcName.empty());

    bool pureVirtual = method.mPure && method.mVirtual;

    const auto& pathID = locate(method.mReturnType.mTypePath, g);
    const auto& returnType = g.getDependentCppName("", pathID, scratch, scratch);

    bool hasPtr{ method.mReturnType.mPointer };
    registerPointer(method.mReturnType, registerPtrs, g, scratch);

    std::string_view prefix = method.mReturnType.mConst ? "const " : "";
    std::string_view suffix = hasPtr ? "*" : "";
    if (method.mReturnType.mReference) {
        suffix = "&";
    }

    oss << "\n\t.function(\"" << funcName << "\", select_overload<"
        << prefix << returnType
        << suffix << "(";
    bool bFirst{ true };
    for (const auto& param : method.mParameters) {
        const auto& pathID = locate(param.mTypePath, g);
        const auto& cppType = g.getDependentCppName("", pathID, scratch, scratch);
        hasPtr |= param.mPointer;
        if (bFirst) {
            bFirst = false;
        } else {
            oss << ", ";
        }

        if (param.mConst) {
            oss << "const ";
        }

        oss << cppType;

        if (param.mReference) {
            oss << "&";
        }

        if (param.mPointer) {
            oss << "*";
        }
    }
    std::string_view constSymb = method.mConst ? "const" : "";

    oss << ")" << constSymb << ">" << std::format("(&{0}::{1})", dependentCppName, funcName);
    if (pureVirtual) {
        oss << ", pure_virtual()";
    }
    if (hasPtr) {
        oss << ", allow_raw_pointers()";
    }
    oss << ")";
}

void outputClass(
    std::ostream& bindingOss,
    std::pmr::unordered_set<std::pmr::string>& registerPtrs,
    pmr_ostringstream& forwardDefs,
    const std::pmr::string& name,
    const Struct& s,
    const SyntaxGraph& g,
    const auto& vertID,
    std::pmr::memory_resource* scratch) {
    pmr_ostringstream oss2(std::ios_base::out, scratch);

    const auto& typePath = g.getTypePath(vertID, scratch);
    const auto& dependentCppName = g.getDependentCppName("", vertID, scratch, scratch);
    oss2 << "\n";
    oss2 << "class_<" << dependentCppName;

    const auto& inherit = get(g.inherits, g, vertID);
    for (const auto& base : inherit.mBases) {
        auto memberID = locate(base.mTypePath, g);
        auto memberType = g.getDependentCppName("", memberID, scratch, scratch);
        oss2 << ", base<" << memberType << ">";
    }

    oss2 << R"(>(")" << name << R"("))"
         << "\n";

    if (g.isPmr(vertID)) {
        oss2 << std::vformat("\t.constructor<>(&creator<{}>)\n", std::make_format_args(dependentCppName));
    }

    for (const auto& con : s.mConstructors) {
        oss2 << "\t.constructor<";
        bool bFirst{ true };
        for (const auto& id : con.mIndices) {
            for (uint32_t i = 0; const auto& m : s.mMembers) {
                if (i == id) {
                    auto memberID = locate(m.mTypePath, g);
                    auto memberType = g.getDependentCppName("", memberID, scratch, scratch);
                    if (bFirst) {
                        bFirst = false;
                    } else {
                        oss2 << ", ";
                    }
                    oss2 << memberType;
                }
            }
        }
        oss2 << ">()\n";
    }

    const auto maxParams2 = 2;
    const auto& methods = s.mMethods;
    for (uint32_t methodID = 0; const auto& method : methods) {
        if (method.mSkip || method.mSetter) {
            ++methodID;
            continue;
        }

        if (method.mGetter) {
            std::pmr::string setter{ "", scratch };
            if (methodID + 1 < methods.size()) {
                const auto& nextMethod = methods[methodID + 1];
                if (nextMethod.mSetter) {
                    // setter = nextMethod.mFunctionName;
                    setter = dependentCppName + "::" + nextMethod.mFunctionName;
                }
            }
            const auto& memberName = camelToVariable(method.mFunctionName, scratch);
            auto getter = dependentCppName + "::" + method.mFunctionName;
            outputClassProperty(oss2, name, dependentCppName, memberName, getter, setter, scratch);
            registerPointer(method.mReturnType, registerPtrs, g, scratch);

        } else if (method.mSetter) {
            if (methodID && (methodID - 1 < methods.size())) {
                const auto& prevMethod = methods[methodID - 1];
                if (!prevMethod.mGetter) {
                    // only setter is not support
                    Expects(false);
                }
            }
        } else {
            oututClassFunction(oss2, registerPtrs, dependentCppName, method, g, scratch);
        }
        ++methodID;
    }

    const auto& members = s.mMembers;
    if (!members.empty()) {
        for (const auto& member : members) {
            if (member.mPublic) {
                const auto& memberName = member.getMemberName();
                std::pmr::string getterStr(name + "_" + memberName + "_getter", scratch);
                std::pmr::string setterStr(name + "_" + memberName + "_setter", scratch);
                std::string_view setter{};
                if (!member.mConst) {
                    setter = setterStr;
                }

                outputClassProperty(oss2, name, dependentCppName, memberName, getterStr, setter, scratch);
                defineGetter(forwardDefs, dependentCppName, name, memberName, scratch);
                defineSetter(forwardDefs, dependentCppName, name, member, g, scratch);
                registerPointer(member, registerPtrs, g, scratch);
            }
        }
    }
    oss2 << ";";
    copyString(bindingOss, oss2.str());
};

void specializeForEMS(std::ostream& forwardDefs,
    const std::pmr::vector<Member>& elements,
    std::string_view instType,
    const SyntaxGraph& g,
    std::pmr::memory_resource* scratch) {

    forwardDefs << std::vformat(R"(
template <>
emscripten::val toEMS(const {0}& v) {{
    emscripten::val obj = emscripten::val::object();)",
        std::make_format_args(instType));
    for (const auto& ele : elements) {
        forwardDefs << std::vformat(R"(
    obj.set("{0}", v.{0});)",
            std::make_format_args(ele.getMemberName()));
    }
    forwardDefs << "\n\treturn obj;\n}\n";

    forwardDefs << std::vformat(R"(
template <>
{0} fromEMS(const emscripten::val& v) {{
    auto obj = creator<{0}>();)",
        std::make_format_args(instType));
    for (const auto& ele : elements) {
        const auto& pathID = locate(ele.mTypePath, g);
        const auto& memType = g.getDependentCppName("", pathID, scratch, scratch);
        if (ele.mPointer) {
            forwardDefs << std::vformat(R"(
    obj.{0} = v["{0}"].as<{1}>();)",
                std::make_format_args(ele.getMemberName(), memType));
        } else {
            forwardDefs << std::vformat(R"(
    assignVal(obj.{0}, v["{0}"].as<{1}>());)",
                std::make_format_args(ele.getMemberName(), memType));
        }
    }
    forwardDefs << "\n\treturn obj;\n}\n";

    forwardDefs << std::vformat(R"(
namespace emscripten::internal {{

template <>
struct BindingType<const {0}&> {{
    typedef typename BindingType<val>::WireType WireType;

    static WireType toWireType(const {0}& v) {{
        return BindingType<val>::toWireType(toEMS(v));
    }}
    static {0} fromWireType(WireType v) {{
        const emscripten::val& ev = BindingType<val>::fromWireType(v);
        return fromEMS<{0}>(ev);
    }}
}};

}}
)",
        std::make_format_args(instType));
}

void defineAssign(std::ostream& forwardDefs,
    const Member& member,
    std::pmr::unordered_set<std::pmr::string>& copySet,
    const SyntaxGraph& g,
    std::pmr::memory_resource* scratch) {
    const auto& memberTypePath = member.mTypePath;
    const auto& prop = member.getMemberName();
    auto pathID = locate(memberTypePath, g);
    const auto& memberTypename = g.getDependentCppName("", pathID, scratch, scratch);

    if (copySet.count(memberTypename)) {
        return;
    }
    copySet.insert(memberTypename);

    std::string_view instName = memberTypePath;
    while (instName.ends_with('>')) {
        instName = peel(instName);
    }

    auto idx = instName.find_first_of(',');
    if (instName.find_first_of(',') != std::string::npos) {
        instName = instName.substr(idx + 1);
    }

    auto instPathID = locate(instName, g);

    bool isPmr{ g.isPmr(pathID) }, isVec{ false }, isMap{ false };
    if (instPathID != g.null_vertex()) {
        isPmr = g.isPmr(instPathID);
        isVec = g.isTypescriptArray(pathID, scratch);
        isMap = g.isTypescriptMap(pathID);
    }

    if (isPmr && isVec) {
        const auto& instType = g.getDependentCppName("", instPathID, scratch, scratch);
        const auto& traits = get(g.traits, g, instPathID);
        const auto& s = get_if<Struct>(instPathID, &g);
        if ((traits.mFlags & NO_COPY) || (traits.mFlags & NO_MOVE_NO_COPY)) {
            const auto& s = get_if<Struct>(instPathID, &g);
            const auto& elements = s->mMembers;
            specializeForEMS(forwardDefs, elements, instType, g, scratch);
        }
        forwardDefs << std::vformat(R"(
template<>
void assignVal({0}& lhs, const {0}& rhs) {{
    if(!lhs.empty()) {{
        lhs.clear();
    }}
    lhs.reserve(rhs.size());
    for (const auto& ele : rhs) {{
        lhs.emplace_back(cloneCustom(ele));
    }}
}}
)",
            std::make_format_args(memberTypename));
    }

    if (isPmr && isMap) {
        const auto& instType = g.getDependentCppName("", instPathID, scratch, scratch);
        const auto& traits = get(g.traits, g, instPathID);
        if ((traits.mFlags & NO_COPY) || (traits.mFlags & NO_MOVE_NO_COPY)) {
            const auto& s = get_if<Struct>(instPathID, &g);
            const auto& elements = s->mMembers;
            specializeForEMS(forwardDefs, elements, instType, g, scratch);
        }

        forwardDefs << std::vformat(R"(
template<>
void assignVal({0}& lhs, const {0}& rhs) {{
    if(!lhs.empty()) {{
        lhs.clear();
    }}
    for (const auto& ele : rhs) {{
        lhs.emplace(std::piecewise_construct_t{{}},
                         std::forward_as_tuple(ele.first),   
                         std::forward_as_tuple(cloneCustom(ele.second)));
    }}
}}
)",
            std::make_format_args(memberTypename));
    }
}

void defineAssign(pmr_ostringstream& forwardDefs, std::string_view typePath,
    std::pmr::unordered_set<std::pmr::string>& copySet,
    const SyntaxGraph& g, std::pmr::memory_resource* scratch) {
    const auto& pathID = locate(typePath, g);
    const auto& className = g.getDependentCppName("", pathID, scratch, scratch);
    const auto* tp = get_if<Struct>(pathID, &g);
    Expects(tp);
    const auto& s = *tp;

    if (copySet.count(className)) {
        return;
    }
    copySet.insert(className);

    if (!s.mMembers.empty()) {
        for (const auto& member : s.mMembers) {
            defineAssign(forwardDefs, member, copySet, g, scratch);
        }
        forwardDefs << std::vformat(R"(
template<>
void assignVal({0}& t, const {0}& v) {{
)",
            std::make_format_args(className));
        for (const auto& member : s.mMembers) {

            forwardDefs << std::vformat(
                R"(    assignVal(t.{0}, v.{0});
)",
                std::make_format_args(member.getMemberName()));
        }
    } else {
        if (g.isPmr(pathID)) {
            forwardDefs << std::vformat(R"(
template<>
void assignVal({0}& t, const {0}& v) {{
    tv;
}};
)",
                std::make_format_args(className));
        } else {
            forwardDefs << std::vformat(R"(
template<>
void assignVal({0}& t, const {0}& v) {{
    t = v;
}};
)",
                std::make_format_args(className));
        }
    }

    forwardDefs << "}\n";
}

void generateBindings(std::ostream& bindingsOss,
    std::pmr::unordered_set<std::pmr::string>& registerPtrs,
    std::pmr::unordered_set<std::pmr::string>& nonDefaultCtorObjects,
    std::pmr::unordered_set<std::pmr::string>& nonDefaultCopyCtorObjects,
    pmr_ostringstream& forwardDefs,
    uint32_t moduleID,
    const ModuleBuilder& builder,
    std::pmr::memory_resource* scratch) {

    std::string_view ns = "";
    const auto& g = builder.mSyntaxGraph;
    const auto& mg = builder.mModuleGraph;

    std::pmr::unordered_set<std::pmr::string> copySet{};
    for (const auto& vertID : make_range(vertices(g))) {
        const auto& modulePath = get(g.modulePaths, g, vertID);
        auto moduleID1 = locate(modulePath, mg);
        if (moduleID1 != moduleID)
            continue;

        const auto& g = builder.mSyntaxGraph;
        const auto& traits = get(g.traits, g, vertID);
        const auto& comment = get(g.comments, g, vertID);

        auto name = g.getTypescriptTypename(vertID, scratch, scratch);
        if (traits.mImport)
            continue;

        if (traits.mFlags & IMPL_DETAIL)
            continue;

        if (!traits.mExport) {
            continue;
        }

        if (g.isTypescriptData(name))
            continue;

        auto cppName = g.getDependentCppName(ns, vertID, scratch, scratch);
        auto tsName = g.getTypescriptTypename(vertID, scratch, scratch);

        if ((traits.mFlags & PMR_DEFAULT) && (!(traits.mFlags & NO_COPY) || !(traits.mFlags & NO_MOVE_NO_COPY))) {
            nonDefaultCopyCtorObjects.emplace(cppName);
        }

        bool bPmr = g.isPmr(vertID); // is_default_constructible

        if (bPmr) {
            nonDefaultCtorObjects.emplace(cppName);
        }

        visit_vertex(
            vertID, g,
            [&](const Struct& s) {
                const auto& inherits = get(g.inherits, g, vertID);
                if (bPmr) {
                    const auto& typePath = g.getTypePath(vertID, scratch);
                    defineAssign(forwardDefs, typePath, copySet, g, scratch);
                }
                std::pmr::vector<std::pmr::string> bases(scratch);
                bases.reserve(inherits.mBases.size());
                for (const auto& base : inherits.mBases) {
                    bases.emplace_back(base.mTypePath);
                }
                if (s.mMemberFunctions.empty() && s.mMethods.empty() && !bPmr) {
                    outputStruct(bindingsOss, registerPtrs, cppName, name, s.mMembers, g, scratch);
                } else {
                    outputClass(bindingsOss, registerPtrs, forwardDefs, name, s, g, vertID, scratch);
                }
            },
            [&](const Variant& s) {
                if (!g.isTag(vertID))
                    return;

                bindingsOss << "\n";
                bindingsOss << "bool nativevalue_to_se(const " << cppName
                            << " &from, se::Value &to, se::Object *ctx); // NOLINT\n";
            },
            [&](const auto& s) {
            });
    }
}

} // namespace

void Cocos::Meta::generateWASMExports(std::ostream& ossMain,
    std::pmr::string& space,
    uint32_t moduleID,
    const ModuleBuilder& builder,
    std::string_view scope,
    std::pmr::set<std::pmr::string>& imports,
    std::pmr::memory_resource* scratch) {
    const auto& g = builder.mSyntaxGraph;
    const auto& mg = builder.mModuleGraph;

    pmr_ostringstream bindingOss(std::ios_base::out, scratch);
    std::pmr::unordered_set<std::pmr::string> registerPtrs(scratch);
    std::pmr::unordered_set<std::pmr::string> nonDefaultCtorObjects(scratch);
    std::pmr::unordered_set<std::pmr::string> nonDefaultCopyCtorObjects(scratch);
    pmr_ostringstream forwardDefs(std::ios_base::out, scratch);

    generateBindings(bindingOss, registerPtrs, nonDefaultCtorObjects, nonDefaultCopyCtorObjects, forwardDefs, moduleID, builder, scratch);

    const auto& moduleInfo = get(builder.mModuleGraph.modules, builder.mModuleGraph, moduleID);

    {
        // headers
        ossMain << R"(
#pragma once
#include <emscripten/bind.h>)";

        const auto& moduleName = get(ModuleGraph::names, mg, moduleID);
        if (moduleName != "RenderInterface") {
            ossMain << moduleInfo.mHeader;
        }
        ossMain << std::format(R"(#include "
WasmDefine.h"
#include "{}Types.h")",
            moduleInfo.mFilePrefix);

        if (!registerPtrs.empty()) {
            ossMain << "\n";
            ossMain << "REGISTER_PTRS(";

            std::string_view seperator = "";
            for (const auto& typeName : registerPtrs) {
                ossMain << seperator << typeName;
                seperator = ", ";
            }
            ossMain << ");";
        }

        if (!nonDefaultCtorObjects.empty() || nonDefaultCopyCtorObjects.empty()) {
            ossMain << "\n";
            static constexpr const char* copyTmplStr = R"(
template<>
{0} cloneCustom<{0}>(const {0}& lhs) {{
    return {0}{{lhs, boost::container::pmr::get_default_resource()}};
}};
)";
            static constexpr const char* ctorTmplStr = R"(
template <>
{0} creator<{0}>() {{
    return {0}{{boost::container::pmr::get_default_resource()}};
}};
)";

            for (const auto& typeName : nonDefaultCopyCtorObjects) {
                ossMain << "\n";
                ossMain << std::vformat(copyTmplStr, std::make_format_args(typeName));
            }

            if (!forwardDefs.str().empty()) {
                copyString(ossMain, forwardDefs.str());
            }
        }

        if (!bindingOss.str().empty()) {
            ossMain << std::vformat(R"(
EMSCRIPTEN_BINDINGS({0}_WASM_EXPORT) {{
using emscripten::value_object;
using emscripten::class_;
using emscripten::select_overload;
using emscripten::base;
using emscripten::allow_raw_pointers;
using emscripten::pure_virtual;

)",
                std::make_format_args(moduleName));
            copyString(ossMain, bindingOss.str());
            ossMain << std::vformat(R"(
}}; // EMSCRIPTEN_BINDINGS({0}_WASM_EXPORT)
)",
                std::make_format_args(moduleName));
        }
    }
}

}