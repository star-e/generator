#include "BuilderUtils.h"

namespace Cocos::Meta {

void outputComment(std::ostream& oss, std::pmr::string& space, std::string_view comment) {
    OSS << "/**\n";
    std::istringstream iss{ std::string{ comment } };
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) {
            OSS << " *\n";
            continue;
        }
        OSS << " * " << line << "\n";
    }
    OSS << " */\n";
}

void outputEnumComment(std::ostream& oss, std::pmr::string& space,
    const SyntaxGraph& g, const SyntaxGraph::vertex_descriptor vertID,
    const EnumValue& e) {
    outputComment(oss, space, e.mComment);
}

void outputMethodComment(std::ostream& oss, std::pmr::string& space,
    const SyntaxGraph& g, const SyntaxGraph::vertex_descriptor vertID,
    const Method& method) {
    if (!any(method.mFlags) && method.mComment.empty()) {
        return;
    }
    OSS << "/**\n";
    if (any(method.mFlags & Doc::Beta)) {
        OSS << " * @beta Feature is under development\n";
    }
    if (any(method.mFlags & Doc::Experimental)) {
        OSS << " * @experimental\n";
    }
    if (any(method.mFlags & Doc::Deprecated)) {
        OSS << " * @deprecated Method will be removed in 3.9.0\n";
    }
    std::istringstream iss{ std::string{ method.mComment } };
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) {
            OSS << " *\n";
            continue;
        }
        OSS << " * " << line << "\n";
    }
    OSS << " */\n";
}

}
