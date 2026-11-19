#pragma once

#include "generated/TypeGraphFwd.h"

#include <filesystem>
#include <map>
#include <set>
#include <string>

namespace cc::codegen {

struct SettingsBinding {
    std::set<std::string> interfaceOnly;
    std::set<std::string> registerEnums;
    std::map<std::string, std::map<std::string, std::string>> renamedFrom;
};

std::string emitSettings(const TypeGraph& graph, ModuleId owner, const SettingsBinding& binding,
                         const std::filesystem::path& typescript, const std::string& preamble);

} // namespace cc::codegen
