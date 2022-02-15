#pragma once
#include <Cocos/AST/BuilderFwd.h>

namespace Cocos::Meta {

std::pmr::string generateToJsIni(const ModuleBuilder& builder, uint32_t moduleID);

}
