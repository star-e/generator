#include "ToJsBuilder.h"
#include "BuilderTypes.h"
#include "SyntaxGraphs.h"
#include "SyntaxUtils.h"

namespace Cocos::Meta {

std::pmr::string generateToJsIni(const ModuleBuilder& builder, uint32_t moduleID) {
    auto scratch = builder.mScratch;
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    const auto& g = builder.mSyntaxGraph;
    const auto& mg = builder.mModuleGraph;

    const auto& m = get(mg.modules, mg, moduleID);

    OSS << "[" << m.mToJsPrefix << "]\n";
    oss << R"(# the prefix to be added to the generated functions. You might or might not use this in your own
# templates
)";

    OSS << "prefix = " << m.mToJsPrefix << "\n";

    oss << R"(
# create a target namespace (in javascript, this would create some code like the equiv. to `ns = ns || {}`)
# all classes will be embedded in that namespace
)";

    OSS << "target_namespace = " << m.mToJsNamespace << "\n";

    OSS << R"(
macro_judgement  =

android_headers =

android_flags = -target armv7-none-linux-androideabi -D_LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS -DANDROID -D__ANDROID_API__=14 -gcc-toolchain %(gcc_toolchain_dir)s --sysroot=%(androidndkdir)s/platforms/android-14/arch-arm  -idirafter %(androidndkdir)s/sources/android/support/include -idirafter %(androidndkdir)s/sysroot/usr/include -idirafter %(androidndkdir)s/sysroot/usr/include/arm-linux-androideabi -idirafter %(clangllvmdir)s/lib64/clang/5.0/include -I%(androidndkdir)s/sources/cxx-stl/llvm-libc++/include

clang_headers =
clang_flags = -nostdinc -x c++ -std=c++17 -fsigned-char -mfloat-abi=soft -U__SSE__

cocos_headers = -I%(cocosdir)s -I%(cocosdir)s/cocos -I%(cocosdir)s/cocos/renderer -I%(cocosdir)s/cocos/platform/android -I%(cocosdir)s/external/sources
cocos_flags = -DANDROID -DCC_PLATFORM=3 -DCC_PLATFORM_IOS=1 -DCC_PLATFORM_MACOS=4 -DCC_PLATFORM_WINDOWS=2 -DCC_PLATFORM_ANDROID=3


cxxgenerator_headers =

# extra arguments for clang
extra_arguments = %(android_headers)s %(clang_headers)s %(cxxgenerator_headers)s %(cocos_headers)s %(android_flags)s %(clang_flags)s %(cocos_flags)s %(extra_flags)s

# what headers to parse
)";

    OSS << "headers = %(cocosdir)s/" << m.mFolder << "/" << m.mFilePrefix << "Types.h\n";
    oss << "\n";
    OSS << "hpp_headers =" << m.mToJsHppHeaders << "\n";
    oss << "\n";
    OSS << "cpp_headers =" << m.mToJsCppHeaders;
    {
        auto modulePath = get_path(moduleID, mg, scratch);
        auto imported = g.getImportedTypes(modulePath, scratch);
        for (const auto& [depPath, types] : imported) {
            auto depID = locate(depPath, mg);
            const auto& dep = get(mg.modules, mg, depID);

            for (const auto& type : types) {
                auto typeID = locate(type, g);
                Expects(g.isJsb(typeID, mg));
            }

            const auto& depTraits = get(mg.modules, mg, depID);
            if (depTraits.mFeatures & Jsb) {
                Expects(!dep.mFolder.empty());
                Expects(!dep.mFilePrefix.empty());
                Expects(!dep.mFilePrefix.ends_with(".h"));
                oss << " " << dep.mFolder << "/" << dep.mFilePrefix << "Jsb.h";
            }
        }
    }
    oss << "\n";

    OSS << R"(
replace_headers =

# what classes to produce code for. You can use regular expressions here. When testing the regular
# expression, it will be enclosed in "^$", like this: "^Menu.*$".

)";

    OSS << "classes =";
    auto outputNames = [&](bool bInterface) {
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

            if (bInterface && !traits.mInterface)
                continue;

            const auto& name = get(g.names, g, vertID);

            visit_vertex(
                vertID, g,
                [&](const Composition_ auto& s) {
                    oss << " " << name;
                },
                [&](const auto&) {

                });
        }
    };

    auto outputMethods = [&](SyntaxGraph::vertex_descriptor vertID, bool convert, auto comp) -> std::pmr::string {
        pmr_ostringstream oss(std::ios_base::out, scratch);

        const auto& name = get(g.names, g, vertID);
        visit_vertex(
            vertID, g,
            [&](const Struct& s) {
                std::pmr::set<std::pmr::string> methods(scratch);
                int count = 0;
                for (const auto& method : s.mMethods) {
                    if (!comp(method))
                        continue;
                        
                    if (count == 0) {
                        oss << name << "::[";
                    }
                    if (convert) {
                        auto methodName = method.mFunctionName.substr(3);
                        methodName = camelToVariable(methodName, scratch);
                        if (!methods.contains(methodName)) {
                            if (count)
                                oss << " ";
                            oss << methodName;
                            methods.emplace(methodName);
                        }
                    } else {
                        if (!methods.contains(method.mFunctionName)) {
                            if (count)
                                oss << " ";
                            oss << method.mFunctionName;
                            methods.emplace(method.mFunctionName);
                        }
                    }
                    ++count;
                }
                if (count) {
                    oss << "]";
                }
            },
            [&](const auto&) {

            });
        return oss.str();
    };

    outputNames(false);
    oss << "\n";

    OSS << R"(
classes_need_extend =

# what should we skip? in the format ClassName::[function function]
# ClassName is a regular expression, but will be used like this: "^ClassName$" functions are also
# regular expressions, they will not be surrounded by "^$". If you want to skip a whole class, just
# add a single "*" as functions. See bellow for several examples. A special class name is "*", which
# will apply to all class names. This is a convenience wildcard to be able to skip similar named
# functions from all classes.

)";
    {
        OSS << "skip =";
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

            auto line = outputMethods(vertID, false, [](const Method& method) {
                return method.mSkip;
            });
            if (!line.empty()) {
                if (count++) {
                    oss << ",\n";
                    oss << "      ";
                }
                oss << " " << line;
            }
        }
        oss << "\n";
    }

    OSS << R"(
skip_public_fields =

)";

    OSS << "getter_setter =";
    {
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
            auto line = outputMethods(vertID, true, [&](const Method& method) {
                if (method.mGetter || method.mSetter) {
                    if (functions.contains(method.mFunctionName.substr(3))) {
                        return false;
                    }
                    functions.emplace(method.mFunctionName.substr(3));
                    return true;
                }
                return false;
            });

            if (!line.empty()) {
                if (count++) {
                    oss << ",\n";
                    oss << "               ";
                }
                oss << " " << line;
            }
        }
    }
    oss << "\n";

    OSS << R"(
rename_functions =

rename_classes =

# for all class names, should we remove something when registering in the target VM?
remove_prefix =

# classes for which there will be no "parent" lookup
classes_have_no_parents =

# base classes which will be skipped when their sub-classes found them.
base_classes_to_skip =

# classes that create no constructor
# Set is special and we will use a hand-written constructor
)";
    OSS << "abstract_classes =";
    outputNames(true);
    oss << "\n";
    OSS << R"(
persistent_classes =

classes_owned_by_cpp =

obtain_return_value =
)";
    copyString(oss, space, m.mToJsConfigs);
    return oss.str();
}

}
