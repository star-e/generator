#pragma once

#include "codegen/BoundField.h"
#include "codegen/TypeScriptImports.h"

#include <iosfwd>
#include <set>
#include <string>
#include <string_view>

namespace cc::codegen {

bool hasMoveOnlyValue(const TypeGraph& graph, TypeId id);
// Returns the storage reset statements for an in-place native archive load.
std::string emitCppGraph(const TypeGraph& graph, TypeId id, std::ostream& header, std::ostream& source,
                         std::set<std::string>& headers, std::set<const TypeNode*>& forwardTypes,
                         const std::vector<BoundField>& fields, const std::string& methods, const std::string& inputArchive,
                         bool transactionalLoad);
void emitTypeScriptGraph(const TypeGraph& graph, TypeId id, std::ostream& out, TypeImports& valueImports, TypeImports& typeImports,
                         const std::vector<BoundField>& fields, std::string_view classMethods, std::set<std::string> memberNames,
                         bool compactGraphNames, bool transactionalLoad);

} // namespace cc::codegen
