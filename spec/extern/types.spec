extern module EngineTypes;
import Containers;

namespace std {
    header "string" {
        @string class string;
    }

    header "string_view" {
        struct string_view;
    }

    header "memory" {
        template<T> class shared_ptr;
    }

    header "vector" {
        @container("array") template<T> class vector;
    }
}

namespace boost::container::pmr {
    header "boost/container/pmr/memory_resource.hpp" {
        class memory_resource;
    }
}

namespace cc {
    header "cocos/core/assets/{name}.h" {
        class Material;
        class Texture2D;
    }

    header "cocos/base/Ptr.h" {
        template<T> class IntrusivePtr;
    }
}

namespace cc::render {
    header "cocos/renderer/pipeline/custom/Array4.h" {
        template<T> struct Array4;
    }
}

namespace cc::geometry {
    header "cocos/core/geometry/{name}.h" {
        class Frustum;
        class AABB;
    }
}

namespace cc::pipeline {
    header "cocos/renderer/pipeline/{name}.h" {
        class GeometryRenderer;
        class PipelineSceneData;
    }

    header "cocos/renderer/pipeline/GlobalDescriptorSetManager.h" {
        class GlobalDSManager;
    }

    header "cocos/renderer/pipeline/InstancedBuffer.h" {
        class InstancedBuffer;
    }
}

binding typescript {
    ccstd::unordered_map<ccstd::string, uint32_t> => "Record<string, number>";
    cc::Material => Material from "cocos/asset/assets";
    cc::Texture2D => Texture2D from "cocos/asset/assets";
    std::string_view => "string";
    cc::pipeline::GlobalDSManager => GlobalDSManager from "cocos/rendering/global-descriptor-set-manager";
    cc::pipeline::GeometryRenderer => GeometryRenderer from "cocos/rendering/geometry-renderer";
    cc::pipeline::PipelineSceneData => PipelineSceneData from "cocos/rendering/pipeline-scene-data";
    ccstd::vector<uint8_t> => Uint8Array;
}
