#pragma once

#include "generated/TypeGraphFwd.h"

#include <map>
#include <set>
#include <string>

namespace cc::codegen {

struct SettingsProjection {
    std::set<std::string> interfaceOnly;
    std::set<std::string> registerEnums;
    std::map<std::string, std::map<std::string, std::string>> renamedFrom;
};

std::string emitSettings(const TypeGraph& graph, ModuleId owner, const SettingsProjection& projection,
                         const std::string& preamble);

} // namespace cc::codegen
