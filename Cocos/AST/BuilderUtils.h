#pragma once
#include "SyntaxTypes.h"

namespace Cocos::Meta {

void outputComment(std::ostream& oss, std::pmr::string& space, std::string_view comment);
void outputMethodComment(std::ostream& oss, std::pmr::string& space,
    const SyntaxGraph& g, const SyntaxGraph::vertex_descriptor vertID,
    const Method& method);
void outputEnumComment(std::ostream& oss, std::pmr::string& space,
    const SyntaxGraph& g, const SyntaxGraph::vertex_descriptor vertID,
    const EnumValue& e);

}
