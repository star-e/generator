#pragma once

#include "codegen/BoundField.h"
#include "codegen/TypeScriptImports.h"

#include <iosfwd>
#include <set>
#include <string>
#include <string_view>

namespace cc::codegen {

struct ArchiveOutput;

bool hasMoveOnlyValue(const TypeGraph& graph, TypeId id);
// Returns the statements that clear graph storage or commit a transactional native load.
std::string emitCppGraph(const TypeGraph& graph, TypeId id, std::ostream& header, std::ostream& templateDefinitions, std::ostream& source,
                         std::set<std::string>& headers, std::set<const TypeNode*>& forwardTypes,
                         const std::vector<BoundField>& fields, const std::string& methods, const ArchiveOutput* archive);
void emitTypeScriptGraph(const TypeGraph& graph, TypeId id, std::ostream& out, TypeImports& valueImports, TypeImports& typeImports,
                         const std::vector<BoundField>& fields, std::string_view classMethods, std::set<std::string> memberNames,
                         bool compactGraphNames, bool transactionalLoad);

} // namespace cc::codegen
