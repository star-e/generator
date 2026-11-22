#pragma once

#include "codegen/InterfaceEmitter.h"
#include "generated/TypeGraphTypes.h"
#include "generated/WorkspaceTypes.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cc::codegen {

struct ArchiveOutput {
    const RecordOutput& output;
    const TypeModule& module;
};

struct ResetArgument {
    TypeId type;
    std::string expression;
};
using NestedResets = std::map<uint32_t, std::vector<ResetArgument>>;

void emitRecordJsb(Files& files, const TypeGraph& graph, const TypeModule& module,
                   const RecordOutput& output, const std::string& preamble);

Files emitRecords(const TypeGraph& graph, ModuleId owner, const RecordOutput& output, std::optional<ArchiveOutput> archive,
                  const std::string& preamble, NestedResets& nestedResets);

} // namespace cc::codegen
