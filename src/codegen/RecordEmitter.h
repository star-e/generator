#pragma once

#include "codegen/InterfaceEmitter.h"
#include "generated/TypeGraphTypes.h"
#include "model/Binding.h"
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace cc::codegen {

struct RecordOutput {
    std::filesystem::path nativeRoot;
    std::filesystem::path header;
    std::optional<std::filesystem::path> forwardHeader;
    std::optional<std::filesystem::path> namesHeader;
    std::vector<std::filesystem::path> importedNamesHeaders;
    std::optional<std::filesystem::path> serializationHeader;
    std::optional<std::filesystem::path> jsbHeader;
    std::set<std::string> jsbRecords;
    std::set<std::filesystem::path> jsbIncludes;
    std::optional<std::filesystem::path> typescript;
    std::set<std::string> namedEnums;
    std::set<std::string> constEnums;
    std::map<std::string, std::vector<std::string>> constructors;
    std::map<std::string, ResetBinding> resets;
    std::map<std::string, std::map<std::string, TypeScriptImport>> fieldResets;
    std::map<std::string, TypeScriptImport> bases;
    std::map<std::string, std::filesystem::path> externalClasses;
    std::filesystem::path poolModule;
    std::vector<std::pair<std::string, TypeScriptImport>> poolDependencies;
    std::map<std::string, PoolBinding> pools;
};

struct ArchiveOutput {
    const OutputLayout& layout;
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
