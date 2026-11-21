extern module EngineTypes;
import Containers;

namespace std {
    header "memory" {
        template<T> class shared_ptr;
    }

    header "vector" {
        @container("array") template<T> class vector;
    }
}

namespace cc::geometry {
    header "cocos/core/geometry/{name}.h" {
        class Frustum;
        class AABB;
    }
}

namespace cc::pipeline {
    header "cocos/renderer/pipeline/InstancedBuffer.h" {
        class InstancedBuffer;
    }
}

binding typescript {
    ccstd::unordered_map<ccstd::string, uint32_t> => "Record<string, number>";
    cc::Material => Material from "cocos/asset/assets";
    cc::Texture2D => "Texture2D";
    std::string_view => "string";
    ccstd::vector<uint8_t> => Uint8Array;
}
