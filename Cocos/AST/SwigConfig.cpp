#include "SwigConfig.h"
#include "BuilderTypes.h"
#include "SyntaxGraphs.h"
#include "SyntaxUtils.h"

namespace Cocos::Meta {

std::pmr::string generateSwigConfig(const ModuleBuilder& builder, uint32_t moduleID) {
    auto scratch = builder.mScratch;
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    const auto& g = builder.mSyntaxGraph;
    const auto& mg = builder.mModuleGraph;

    const auto& m = get(mg.modules, mg, moduleID);

    std::pmr::string folder(m.mFolder, scratch);
    folder = folder.substr(6);

    copyString(oss, space, R"(// Define module
// target_namespace means the name exported to JS, could be same as which in other modules
// native2d at the last means the suffix of binding function name, different modules should use unique name
// Note: doesn't support number prefix
)");
    OSS << "%module(target_namespace=\"" << m.mToJsNamespace << "\") " << m.mToJsPrefix << "\n";

    copyString(oss, space, R"(
// Disable some swig warnings, find warning number reference here ( https://www.swig.org/Doc4.1/Warnings.html )
#pragma SWIG nowarn=503,302,401,317,402

// Insert code at the beginning of generated header file (.h)
%insert(header_file) %{
#pragma once
#include <type_traits>
#include "bindings/jswrapper/SeApi.h"
#include "bindings/manual/jsb_conversions.h"
)");
    OSS << "#include \"" << folder << "/" << m.mFilePrefix << "Types.h\"\n";
    copyString(oss, space, "%}\n");

    copyString(oss, space, R"(
// Insert code at the beginning of generated source file (.cpp)
%{
)");
    copyString(oss, space, m.mToJsCppHeaders);
    {
        auto modulePath = get_path(moduleID, mg, scratch);
        auto imported = g.getImportedTypes(modulePath, false, scratch);
        for (const auto& [depPath, types] : imported) {
            auto depID = locate(depPath, mg);
            const auto& dep = get(mg.modules, mg, depID);

            for (const auto& type : types.mImportedTypes) {
                auto typeID = locate(type, g);
                Expects(g.isJsb(typeID, mg));
            }

            for (const auto& type : types.mImported) {
                auto typeID = locate(type, g);
                Expects(g.isJsb(typeID, mg));
            }

            const auto& depTraits = get(mg.modules, mg, depID);
            if (depTraits.mFeatures & Jsb) {
                std::pmr::string folder(dep.mFolder, scratch);
                folder = folder.substr(6);
                Expects(!dep.mFolder.empty());
                Expects(!dep.mFilePrefix.empty());
                Expects(!dep.mFilePrefix.ends_with(".h"));
                OSS << "#include \"" << folder << "/" << dep.mFilePrefix << "Jsb.h\"\n";
            }
        }
    }
    copyString(oss, space, m.mToJsUsingNamespace);
    OSS << "%}\n";

    copyString(oss, space, R"(
// ----- Ignore Section ------
// Brief: Classes, methods or attributes need to be ignored
//
// Usage:
//
//  %ignore your_namespace::your_class_name;
//  %ignore your_namespace::your_class_name::your_method_name;
//  %ignore your_namespace::your_class_name::your_attribute_name;
//
// Note: 
//  1. 'Ignore Section' should be placed before attribute definition and %import/%include
//  2. namespace is needed
//
)");
    {
        int count = 0;
        for (const auto& vertID : make_range(vertices(g))) {
            const auto& modulePath = get(g.modulePaths, g, vertID);
            auto moduleID1 = locate(modulePath, mg);
            if (moduleID1 != moduleID)
                continue;

            auto fullName = g.getDependentCppName("", vertID, scratch, scratch);
            const auto& traits = get(g.traits, g, vertID);

            if (traits.mImport)
                continue;

            if (traits.mFlags & IMPL_DETAIL)
                continue;

            visit_vertex(
                vertID, g,
                [&](const Struct& s) {
                    std::pmr::set<std::pmr::string> methods(scratch);
                    for (const auto& method : s.mMethods) {
                        if (method.mSkip) {
                            if (!methods.contains(method.mFunctionName)) {
                                OSS << "%ignore " << fullName << "::" << method.mFunctionName << ";\n";
                                methods.emplace(method.mFunctionName);
                            }
                            continue;
                        }
                        if (!method.mTypescriptFunctionName.empty() &&
                            method.mTypescriptFunctionName != method.mFunctionName) {
                            OSS << "%rename(" << method.mTypescriptFunctionName << ") " << fullName << "::" << method.mFunctionName << ";\n";
                            continue;
                        }
                    }
                },
                [&](const auto&) {

                });
        }
    }

    copyString(oss, space, R"(
// ----- Rename Section ------
// Brief: Classes, methods or attributes needs to be renamed
//
// Usage:
//
//  %rename(rename_to_name) your_namespace::original_class_name;
//  %rename(rename_to_name) your_namespace::original_class_name::method_name;
//  %rename(rename_to_name) your_namespace::original_class_name::attribute_name;
// 
// Note:
//  1. 'Rename Section' should be placed before attribute definition and %import/%include
//  2. namespace is needed

// ----- Module Macro Section ------
// Brief: Generated code should be wrapped inside a macro
// Usage:
//  1. Configure for class
//    %module_macro(CC_USE_GEOMETRY_RENDERER) cc::pipeline::GeometryRenderer;
//  2. Configure for member function or attribute
//    %module_macro(CC_USE_GEOMETRY_RENDERER) cc::pipeline::RenderPipeline::geometryRenderer;
// Note: Should be placed before 'Attribute Section'
)");
    if (!m.mToJsConfigs.empty()) {
        copyString(oss, space, m.mToJsConfigs);
    }

    copyString(oss, space, R"(
// ----- Attribute Section ------
// Brief: Define attributes ( JS properties with getter and setter )
// Usage:
//  1. Define an attribute without setter
//    %attribute(your_namespace::your_class_name, cpp_member_variable_type, js_property_name, cpp_getter_name)
//  2. Define an attribute with getter and setter
//    %attribute(your_namespace::your_class_name, cpp_member_variable_type, js_property_name, cpp_getter_name, cpp_setter_name)
//  3. Define an attribute without getter
//    %attribute_writeonly(your_namespace::your_class_name, cpp_member_variable_type, js_property_name, cpp_setter_name)
//
// Note:
//  1. Don't need to add 'const' prefix for cpp_member_variable_type 
//  2. The return type of getter should keep the same as the type of setter's parameter
//  3. If using reference, add '&' suffix for cpp_member_variable_type to avoid generated code using value assignment
//  4. 'Attribute Section' should be placed before 'Import Section' and 'Include Section'
//
)");
    {
        auto outputParam = [&](const Method& m) {
            std::ostringstream oss;
            auto retID = locate(m.mReturnType.mTypePath, g);
            oss << g.getDependentCppName("", retID, scratch, scratch);
            if (m.mReturnType.mPointer) {
                oss << "*";
            }
            if (m.mReturnType.mReference) {
                oss << "&";
            }
            return oss.str();
        };

        int count = 0;
        for (const auto& vertID : make_range(vertices(g))) {
            const auto& modulePath = get(g.modulePaths, g, vertID);
            auto moduleID1 = locate(modulePath, mg);
            if (moduleID1 != moduleID)
                continue;

            const auto& traits = get(g.traits, g, vertID);

            if (traits.mImport)
                continue;

            if (traits.mFlags & IMPL_DETAIL)
                continue;

            std::pmr::set<std::pmr::string> functions(scratch);
            auto fullName = g.getDependentCppName("", vertID, scratch, scratch);

            visit_vertex(
                vertID, g,
                [&](const Struct& s) {
                    std::pmr::map<std::pmr::string, std::pmr::set<std::pmr::string>> methods(scratch);
                    std::pmr::map<std::pmr::string, uint32_t> methodIDs(scratch);
                    for (uint32_t id = 0; const auto& method : s.mMethods) {
                        if (method.mGetter || method.mSetter) {
                            auto name = method.mFunctionName.substr(3);
                            methods[name].emplace(method.mFunctionName);
                            methodIDs.emplace(name, id);
                        }
                        ++id;
                    }
                    for (const auto& method : s.mMethods) {
                        auto name = method.mFunctionName.substr(3);
                        auto iter = methods.find(name);
                        if (iter == methods.end())
                            continue;
                        const auto& pair = *iter;
                        OSS << "%attribute(" << fullName
                            << ", " << outputParam(method) << ", " << camelToVariable(pair.first, scratch);
                        for (const auto& name : pair.second) {
                            oss << ", " << name;
                        }
                        oss << ");\n";
                        methods.erase(name);
                    }
                },
                [&](const auto&) {

                });
        }
    }
    copyString(oss, space, R"(
// ----- Import Section ------
// Brief: Import header files which are depended by 'Include Section'
// Note: 
//   %import "your_header_file.h" will not generate code for that header file
//

// ----- Include Section ------
// Brief: Include header files in which classes and methods will be bound
)");
    OSS << "%include \"" << folder << "/" << m.mFilePrefix << "Types.h\"\n";

    return oss.str();
}

}
