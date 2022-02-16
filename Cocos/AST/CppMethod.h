#pragma once
#include "BuilderTypes.h"

namespace Cocos::Meta {

Method parseFunction(const ModuleBuilder& builder, std::string_view function);

std::pmr::vector<Method> parseFunctions(const ModuleBuilder& builder, std::string_view functions);

}
