#pragma once
#include "SyntaxTypes.h"
#include "BuilderTypes.h"

namespace Cocos::Meta {

void generateWASMExports(std::ostream& oss, std::pmr::string& space,
    uint32_t moduleID,
    const ModuleBuilder& builder, std::string_view scope,
    std::pmr::set<std::pmr::string>& imports,
    std::pmr::memory_resource* scratch);

}
