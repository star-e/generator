#include "BuilderUtils.h"

namespace Cocos::Meta {

void outputDoc(std::ostream& oss, std::pmr::string& space, Doc flags) {
    if (any(flags)) {
        OSS << "/**\n";
        if (any(flags & Doc::Beta)) {
            OSS << " * @beta function signature might change\n";
        }
        if (any(flags & Doc::Experimental)) {
            OSS << " * @experimental\n";
        }
        if (any(flags & Doc::Deprecated)) {
            OSS << " * @deprecated method will be removed in 3.9.0\n";
        }
        OSS << " */\n";
    }
}

}
