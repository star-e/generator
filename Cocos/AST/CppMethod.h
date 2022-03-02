#pragma once
#include "BuilderTypes.h"

namespace Cocos::Meta {

Method parseFunction(ModuleBuilder& builder, std::string_view function);

std::pmr::vector<Method> parseFunctions(ModuleBuilder& builder, std::string_view functions);

}
