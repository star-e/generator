#pragma once

#include "model/SourceLocation.h"
#include <string>
#include <vector>

namespace cc::codegen {

struct TypeRef {
    SourceLocation location;
    std::string name;
    bool pointer{};
    std::vector<TypeRef> arguments;
    bool constPointee{};
};

} // namespace cc::codegen
