#include "JsbBuilder.h"
#include "BuilderTypes.h"
#include "SyntaxGraphs.h"

namespace Cocos::Meta {

std::pmr::string generateJsbConversions_h(const ModuleBuilder& builder, uint32_t moduleID) {
    auto scratch = builder.mScratch;
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    const auto& g = builder.mSyntaxGraph;
    const auto& mg = builder.mModuleGraph;

    std::string_view ns = "";

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

        if (traits.mInterface)
            continue;

        if (g.isDerived(vertID))
            continue;

        auto cppName = g.getDependentCppName(ns, vertID, scratch, scratch);
        auto tsName = g.getTypescriptTypename(vertID, scratch, scratch);

        visit_vertex(
            vertID, g,
            [&](const Struct& s) {
                oss << "\n";
                OSS << "template <>\n";
                OSS << "bool nativevalue_to_se(const " << cppName
                    << " &from, se::Value &to, se::Object *ctx);\n";
            },
            [&](const Variant& s) {
                if (!g.isTag(vertID))
                    return;

                oss << "\n";
                OSS << "template <>\n";
                OSS << "bool nativevalue_to_se(const " << cppName
                    << " &from, se::Value &to, se::Object *ctx);\n";
            },
            [&](const auto&) {
            });
    }

    return oss.str();
}

std::pmr::string generateJsbConversions_cpp(const ModuleBuilder& builder, uint32_t moduleID) {
    auto scratch = builder.mScratch;
    pmr_ostringstream oss(std::ios::out, scratch);
    std::pmr::string space(scratch);

    const auto& g = builder.mSyntaxGraph;
    const auto& mg = builder.mModuleGraph;

    std::string_view ns = "";

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

        if (traits.mInterface)
            continue;

        if (g.isDerived(vertID))
            continue;

        auto cppName = g.getDependentCppName(ns, vertID, scratch, scratch);
        auto tsName = g.getTypescriptTypename(vertID, scratch, scratch);

        visit_vertex(
            vertID, g,
            [&](const Struct& s) {
                oss << "\n";
                OSS << "template <>\n";
                OSS << "bool nativevalue_to_se(const " << cppName
                    << " &from, se::Value &to, se::Object *ctx) {\n";
                {
                    INDENT();
                    OSS << "se::HandleObject obj(se::Object::createPlainObject());\n";
                    
                    if (!s.mMembers.empty()) {
                        OSS << "se::Value        tmp;\n";
                    }

                    for (const auto& member : s.mMembers) {
                        oss << "\n";
                        OSS << "nativevalue_to_se(from." << member.mMemberName << ", tmp, ctx);\n";
                        OSS << "obj->setProperty(\""
                            << builder.getMemberName(member.mMemberName, member.mPublic)
                            << "\", tmp);\n";
                    }
                    oss << "\n";
                    OSS << "to.setObject(obj);\n";
                    OSS << "return true;\n";
                }
                OSS << "}\n";
            },
            [&](const Variant& s) {
                if (!g.isTag(vertID))
                    return;

                oss << "\n";
                OSS << "template <>\n";
                OSS << "bool nativevalue_to_se(const " << cppName
                    << " &from, se::Value &to, se::Object *ctx) {\n";
                {
                    INDENT();
                    OSS << "to.setInt32(static_cast<int32_t>(from.index()));\n";
                    OSS << "return true;\n";
                }
                OSS << "}\n";
            },
            [&](const auto&) {
            });
    }

    return oss.str();
}

}
