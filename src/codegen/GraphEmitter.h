#pragma once

#include "codegen/BoundField.h"
#include "codegen/TypeScriptImports.h"

#include <iosfwd>
#include <set>
#include <string>

namespace cc::codegen {

bool hasMoveOnlyValue(const TypeGraph& graph, TypeId id);
void emitCppGraph(const TypeGraph& graph, TypeId id, std::ostream& header, std::ostream& source,
                  std::set<std::string>& headers, std::set<std::string>& forwardDeclarations,
                  const std::vector<BoundField>& fields, const std::string& methods, const std::string& inputArchive);
void emitTypeScriptGraph(const TypeGraph& graph, TypeId id, std::ostream& out, TypeImports& imports,
                         const std::vector<BoundField>& fields);

} // namespace cc::codegen
