#pragma once

#include "codegen/TypeScriptImports.h"

namespace cc::codegen {

std::string archivePrimitive(const TypeGraph& graph, TypeId id);
struct ArchiveRequirements {
    bool needsReadSize{};
    bool needsBoolVectorLoad{};

    void checkType(const TypeGraph& graph, ModuleId owner, TypeId id, const std::string& context, bool emitTs);
};
const TypeNode& archiveInterface(const TypeGraph& graph, const TypeModule& module, std::string_view name);
const std::string& archiveMethod(const TypeNode& api, const std::string& method);

// Emit a value at the current archive cursor. A loading caller can request a fresh local.
struct ArchiveScript {
    const TypeGraph& graph;
    const TypeNode& api;
    TypeImports& imports;
    bool save;

    void value(std::ostream& out, TypeId id, const std::string& expression, const std::string& indent,
               bool declare = false, unsigned depth = 0) const;
};

void emitGraphArchive(const TypeGraph& graph, const TypeNode& node, const TypeModule& archive,
                      std::ostream& declarations, std::ostream& definitions, std::ostream* script, TypeImports& imports,
                      ArchiveRequirements& requirements);

} // namespace cc::codegen
