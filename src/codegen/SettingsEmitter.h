#pragma once

#include "generated/TypeGraphFwd.h"
#include "generated/WorkspaceTypes.h"

#include <filesystem>
#include <string>

namespace cc::codegen {

std::string emitSettings(const TypeGraph& graph, ModuleId owner, const SettingsBinding& binding,
                         const std::filesystem::path& typescript, const std::string& preamble);

} // namespace cc::codegen
