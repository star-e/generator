#pragma once

#include "codegen/InterfaceEmitter.h"
#include "generated/TypeGraphTypes.h"
#include "generated/WorkspaceTypes.h"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace cc::codegen {

struct ArchiveOutput {
    const RecordOutput& output;
    const TypeModule& module;
    bool transactionalGraphLoad;
};

struct ResetArgument {
    TypeId type;
    std::string expression;
};
using NestedResets = std::map<uint32_t, std::vector<ResetArgument>>;
// Maps a generated header to the definition headers replaced by forward declarations in its include chain.
using OmittedDefinitionHeaders = std::map<std::string, std::set<std::string>>;

void emitRecordJsb(Files& files, const TypeGraph& graph, const TypeModule& module,
                   const RecordOutput& output, const std::string& preamble);

Files emitRecords(const TypeGraph& graph, ModuleId owner, const RecordOutput& output, std::optional<ArchiveOutput> archive,
                  const std::string& preamble, NestedResets& nestedResets, OmittedDefinitionHeaders& omittedHeaders);

} // namespace cc::codegen
