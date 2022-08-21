#pragma once
#include <Cocos/AST/BuilderFwd.h>

namespace Cocos::Meta {

std::pmr::string generateSwigConfig(const ModuleBuilder& builder, uint32_t moduleID);

} // namespace Cocos::Meta
