#pragma once

#include "codegen/InterfaceEmitter.h"
#include <optional>
#include <set>

namespace cc::codegen {

struct PoolProjection {
    std::string member;
    std::set<std::string> keepOnReset;
};

struct RecordOutput {
    std::filesystem::path nativeRoot;
    std::filesystem::path header;
    std::optional<std::filesystem::path> forwardHeader;
    std::optional<std::filesystem::path> namesHeader;
    std::optional<std::filesystem::path> serializationHeader;
    std::optional<std::filesystem::path> jsbHeader;
    std::set<std::string> jsbRecords;
    std::set<std::filesystem::path> jsbIncludes;
    std::optional<std::filesystem::path> typescript;
    std::set<std::string> namedEnums;
    std::set<std::string> constEnums;
    std::set<std::string> comparable;
    std::set<std::string> hashable;
    std::set<std::string> resetToDefaults;
    std::filesystem::path poolModule;
    std::map<std::string, PoolProjection> pools;
};

struct ArchiveOutput {
    const OutputLayout& layout;
    const TypeModule& module;
};

void emitRecordJsb(Files& files, const TypeGraph& graph, const TypeModule& module,
                   const RecordOutput& output, const std::string& preamble);

Files emitRecords(const TypeGraph& graph, ModuleId owner, const RecordOutput& output, std::optional<ArchiveOutput> archive,
                  const std::string& preamble);

} // namespace cc::codegen
