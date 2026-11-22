#pragma once

#include "generated/TypeGraphFwd.h"
#include "generated/WorkspaceFwd.h"
#include "codegen/TypeScriptImports.h"
#include "model/Binding.h"

#include <filesystem>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <string_view>

namespace cc::codegen {

using Files = std::map<std::filesystem::path, std::string>;

std::string cppMethodDeclarator(std::string type, std::string_view name, bool spaceBeforeReference);
std::string emitHeaderSource(const TypeModule& module, const std::filesystem::path& header, const std::string& preamble);
void emitInterfaceHeaders(Files& files, const TypeGraph& graph, const TypeModule& module, const RecordOutput& output,
                          std::string_view body, const std::set<std::string>& headers, const std::string& preamble);
void emitInterface(const TypeGraph& graph, const TypeNode& node, std::ostream* cpp, std::set<std::string>& headers,
                   std::set<const TypeNode*>& forwardTypes,
                   std::ostream* script, TypeImports& imports, const RecordOutput& output);
void emitImplementationMethods(std::ostream& out, const TypeGraph& graph, const TypeNode& node, std::set<std::string>& headers,
                               bool spaceBeforeReference);

} // namespace cc::codegen
