#pragma once

#include "codegen/InterfaceEmitter.h"

namespace cc::codegen {

struct RecordOutput {
    std::filesystem::path header;
    std::filesystem::path typescript;
    std::filesystem::path poolModule;
    std::map<std::string, std::string> pools;
};

Files emitRecords(const TypeGraph& graph, ModuleId owner, const RecordOutput& output, const std::string& preamble);

} // namespace cc::codegen
