#pragma once

#include "generated/TypeGraphFwd.h"

#include <filesystem>
#include <map>
#include <string>

namespace cc::codegen {

using Files = std::map<std::filesystem::path, std::string>;

struct OutputLayout {
    std::filesystem::path nativeRoot;
    std::filesystem::path includeDir;
    std::string prefix;
    std::filesystem::path typescript;
};

Files emitInterfaces(const TypeGraph& graph, ModuleId module, const OutputLayout& layout,
                     const std::string& preamble);

} // namespace cc::codegen
