#pragma once
#include <Cocos/AST/BuilderFwd.h>

namespace Cocos::Meta {

std::pmr::string generateJsbConversions_h(const ModuleBuilder& builder, uint32_t moduleID);
std::pmr::string generateJsbConversions_cpp(const ModuleBuilder& builder, uint32_t moduleID);

}
