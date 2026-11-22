#pragma once

#include "codegen/TypeScriptImports.h"

namespace cc::codegen {

std::string archivePrimitive(const TypeGraph& graph, TypeId id);
struct ArchiveRequirements {
    bool needsBoolVectorLoad{};

    void checkType(const TypeGraph& graph, ModuleId owner, TypeId id, const std::string& context);
};

struct ArchiveImports {
    TypeImports& values;
    std::string defaultCodecModule;
};

// Emit a value at the current archive cursor. A loading caller can request a fresh local.
struct ArchiveScript {
    const TypeGraph& graph;
    const TypeNode& api;
    ArchiveImports& imports;
    bool save;
    unsigned arrayLengthCount{};

    void value(std::ostream& out, TypeId id, const std::string& expression, const std::string& indent,
               bool declare = false, unsigned depth = 0);
};

void emitGraphArchive(const TypeGraph& graph, const TypeNode& node, const TypeModule& archive,
                      std::ostream& declarations, std::ostream& definitions, std::ostream* script, ArchiveImports& imports,
                      ArchiveRequirements& requirements);

} // namespace cc::codegen
