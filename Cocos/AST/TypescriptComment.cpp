#include "TypescriptBuilder.h"
#include <sstream>

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

}
