#pragma once

#include "generated/TypeGraphFwd.h"

#include <iosfwd>
#include <set>
#include <string>

namespace cc::codegen {

void emitCppGraph(const TypeGraph& graph, TypeId id, std::ostream& header, std::ostream& source,
                  std::set<std::string>& headers, std::set<std::string>& forwardDeclarations);

} // namespace cc::codegen
