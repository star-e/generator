#include "codegen/RecordEmitter.h"
#include "generated/TypeGraphTypes.h"

#include <sstream>
#include <stdexcept>

namespace cc::codegen {

void emitRecordJsb(Files& files, const TypeGraph& graph, const TypeModule& module,
                   const RecordOutput& output, const std::string& preamble) {
    std::ostringstream header, source, toNativeDeclarations, toNativeDefinitions;
    header << preamble << "// clang-format off\n#pragma once\n#include \"cocos/bindings/manual/jsb_conversions.h\"\n"
           << "#include \"" << output.forwardHeader.generic_string() << "\"\n";
    source << preamble << "// clang-format off\n";
    auto includes = output.jsbIncludes;
    includes.insert(*output.jsbHeader);
    includes.insert(output.header);
    for (const auto& path : includes) source << "#include \"" << path.generic_string() << "\"\n";

    auto pending = output.jsbRecords;
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        if (!pending.erase(node.name)) continue;
        const auto* record = std::get_if<RecordType>(&node.data);
        if (!record) throw std::runtime_error(node.qualifiedName + ": expected a JSB record");
        const auto& type = node.qualifiedName;
        const auto toSe = "bool nativevalue_to_se(const " + type + " &from, se::Value &to, se::Object *ctx)";
        header << '\n' << toSe << "; // NOLINT\n";
        source << '\n' << toSe << " { // NOLINT\n"
               << "    se::HandleObject obj(se::Object::createPlainObject());\n"
               << "    se::Value        tmp;\n";
        const auto args = "(const se::Value &from, " + type + " *to, se::Object *ctx)";
        toNativeDeclarations << "template <>\nbool sevalue_to_native" << args << "; // NOLINT\n\n";
        toNativeDefinitions << "\ntemplate <>\nbool sevalue_to_native<" << type << ">" << args << " { // NOLINT\n"
                            << "    SE_PRECONDITION2(from.isObject(), false, \" Convert parameter to " << node.name << " failed !\");\n\n"
                            << "    auto *obj = const_cast<se::Object *>(from.toObject());\n"
                            << "    bool ok = true;\n    se::Value field;\n";
        for (const auto& field : record->fields) {
            if (field.cppPrivate) continue;
            source << "\n    nativevalue_to_se(from." << field.name << ", tmp, ctx);\n"
                   << "    obj->setProperty(\"" << field.name << "\", tmp);\n";
            toNativeDefinitions << "    obj->getProperty(\"" << field.name << "\", &field, true);\n"
                                << "    if(!field.isNullOrUndefined()) {\n"
                                << "        ok &= sevalue_to_native(field, &(to->" << field.name << "), ctx);\n    }\n";
        }
        source << "\n    to.setObject(obj);\n    return true;\n}\n";
        toNativeDefinitions << "    return ok;\n}\n";
    }
    if (!pending.empty()) throw std::runtime_error("unknown JSB record " + *pending.begin() + " in " + module.name);
    header << "\n// if function overload is used, android build fails\n" << toNativeDeclarations.str() << "// clang-format on\n";
    source << toNativeDefinitions.str() << "\n// clang-format on\n";
    auto path = output.nativeRoot / *output.jsbHeader;
    files.emplace(path, header.str());
    files.emplace(path.replace_extension(".cpp"), source.str());
}

} // namespace cc::codegen
