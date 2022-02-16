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

        if (!(traits.mFlags & JSB))
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
                OSS << "bool nativevalue_to_se(const " << cppName
                    << " &from, se::Value &to, se::Object *ctx); // NOLINT\n";
            },
            [&](const Variant& s) {
                if (!g.isTag(vertID))
                    return;

                oss << "\n";
                OSS << "bool nativevalue_to_se(const " << cppName
                    << " &from, se::Value &to, se::Object *ctx); // NOLINT\n";
            },
            [&](const auto&) {
            });
    }

    for (const auto& vertID : make_range(vertices(g))) {
        const auto& modulePath = get(g.modulePaths, g, vertID);
        auto moduleID1 = locate(modulePath, mg);
        if (moduleID1 != moduleID)
            continue;

        const auto& traits = get(g.traits, g, vertID);

        if (traits.mImport)
            continue;

        if (!(traits.mFlags & JSB))
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
                OSS << "bool sevalue_to_native(const se::Value &from, "
                    << cppName << " *to, se::Object *ctx); // NOLINT\n";
            },
            [&](const Variant& s) {
                if (!g.isTag(vertID))
                    return;

                oss << "\n";
                OSS << "bool sevalue_to_native(const se::Value &from, "
                    << cppName << " *to, se::Object *ctx); // NOLINT\n";
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

    bool hasVariantTag = false;

    for (const auto& vertID : make_range(vertices(g))) {
        const auto& modulePath = get(g.modulePaths, g, vertID);
        auto moduleID1 = locate(modulePath, mg);
        if (moduleID1 != moduleID)
            continue;

        const auto& traits = get(g.traits, g, vertID);

        if (traits.mImport)
            continue;

        if (!(traits.mFlags & JSB))
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
                OSS << "bool nativevalue_to_se(const " << cppName
                    << " &from, se::Value &to, se::Object *ctx) { // NOLINT\n";
                {
                    INDENT();
                    OSS << "se::HandleObject obj(se::Object::createPlainObject());\n";
                    
                    if (!s.mMembers.empty()) {
                        OSS << "se::Value        tmp;\n";
                    }

                    for (const auto& member : s.mMembers) {
                        auto memberID = locate(member.mTypePath, g);
                        Expects(g.isJsb(memberID, mg));
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
                if (!g.isTag(vertID)) {
                    Expects(false);
                    return;
                }

                hasVariantTag = true;

                oss << "\n";
                OSS << "bool nativevalue_to_se(const " << cppName
                    << " &from, se::Value &to, se::Object *ctx) { // NOLINT\n";
                {
                    INDENT();
                    OSS << "to.setInt32(static_cast<int32_t>(from.index()));\n";
                    OSS << "return true;\n";
                }
                OSS << "}\n";
            },
            [&](const auto&) {
                Expects(false);
            });
    }

    for (const auto& vertID : make_range(vertices(g))) {
        const auto& modulePath = get(g.modulePaths, g, vertID);
        auto moduleID1 = locate(modulePath, mg);
        if (moduleID1 != moduleID)
            continue;

        const auto& traits = get(g.traits, g, vertID);

        if (traits.mImport)
            continue;

        if (!(traits.mFlags & JSB))
            continue;

        if (traits.mFlags & IMPL_DETAIL)
            continue;

        if (traits.mInterface)
            continue;

        if (g.isDerived(vertID))
            continue;

        auto cppName = g.getDependentCppName(ns, vertID, scratch, scratch);
        auto tsName = g.getTypescriptTypename(vertID, scratch, scratch);
        const auto& name = get(g.names, g, vertID); 
        visit_vertex(
            vertID, g,
            [&](const Struct& s) {
                oss << "\n";
                OSS << "bool sevalue_to_native(const se::Value &from, "
                    << cppName << " *to, se::Object *ctx) { // NOLINT\n";
                {
                    INDENT();
                    OSS << "SE_PRECONDITION2(from.isObject(), false, \" Convert parameter to "
                        << name << " failed !\");\n";

                    oss << "\n";
                    OSS << "auto *obj = const_cast<se::Object *>(from.toObject());\n";
                    OSS << "bool ok = true;\n";
                    OSS << "se::Value field;\n";

                    for (const auto& member : s.mMembers) {
                        auto memberID = locate(member.mTypePath, g);
                        Expects(g.isJsb(memberID, mg));
                        OSS << "obj->getProperty(\""
                            << builder.getMemberName(member.mMemberName, member.mPublic) << "\", &field, true);\n";
                        OSS << "if(!field.isNullOrUndefined()) {\n";
                        {
                            INDENT();
                            OSS << "ok &= sevalue_to_native(field, &(to->" << member.mMemberName
                                << "), ctx);\n";
                        }
                        OSS << "}\n";
                    }
                    OSS << "return ok;\n";
                }
                OSS << "}\n";
            },
            [&](const Variant& s) {
                if (!g.isTag(vertID)) {
                    Expects(false);
                    return;
                }

                oss << "\n";
                OSS << "bool sevalue_to_native(const se::Value &from, "
                    << cppName << " *to, se::Object *ctx) { // NOLINT\n";
                {
                    INDENT();
                    OSS << "SE_PRECONDITION2(from.isNumber(), false, \" Convert parameter to "
                        << name << " failed !\");\n";
                    OSS << "size_t id = from.toInt32();\n";
                    OSS << "*to = cc::variant_from_index<" << cppName << ">(id);\n";
                    OSS << "return true;\n";
                }
                OSS << "}\n";
            },
            [&](const auto&) {
                Expects(false);
            });
    }

    auto result = oss.str();
    if (hasVariantTag) {
        result.insert(0, "#include \"cocos/renderer/pipeline/custom/Overload.h\"\n");
    }
    return result;
}

}
