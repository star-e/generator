module TypeGraphCore;
import SourceLocation;

namespace cc::codegen {
    @comparison("==") struct TypeId { value: uint32_t = 0xFFFFFFFF; }
    @comparison("==") struct ModuleId { value: uint32_t = 0xFFFFFFFF; }

    struct TypeUse {
        type: TypeId;
        location: SourceLocation;
    }
}
