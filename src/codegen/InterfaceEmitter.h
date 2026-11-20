#pragma once

#include "generated/TypeGraphFwd.h"
#include "codegen/TypeScriptImports.h"
#include "model/Binding.h"

#include <filesystem>
#include <map>
#include <string>

namespace cc::codegen {

using Files = std::map<std::filesystem::path, std::string>;

struct OutputLayout {
    std::filesystem::path nativeRoot;
    std::filesystem::path includeDir;
    std::string prefix;
    std::filesystem::path typescript;
    InterfaceBindings interfaceBindings;
};

std::string emitHeaderSource(const TypeModule& module, const std::filesystem::path& header, const std::string& preamble);
Files emitInterfaces(const TypeGraph& graph, ModuleId module, const OutputLayout& layout,
                     const std::string& preamble);
void emitInterface(const TypeGraph& graph, const TypeNode& node, std::ostream& cpp, std::set<std::string>& headers,
                   std::ostream* script, TypeImports& imports, const InterfaceBindings& bindings);

} // namespace cc::codegen
