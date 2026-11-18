#pragma once

#include "codegen/InterfaceEmitter.h"
#include <optional>
#include <set>

namespace cc::codegen {

struct RecordOutput {
    std::filesystem::path header;
    std::optional<std::filesystem::path> typescript;
    std::set<std::string> namedEnums;
    std::set<std::string> constEnums;
    std::filesystem::path poolModule;
    std::map<std::string, std::string> pools;
};

Files emitRecords(const TypeGraph& graph, ModuleId owner, const RecordOutput& output, const std::string& preamble);

} // namespace cc::codegen
