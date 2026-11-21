extern module EngineTypes;
import Containers;

namespace std {
    header "string" {
        @string class string;
    }

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
    cc::Texture2D => "Texture2D";
    std::string_view => "string";
    cc::pipeline::GlobalDSManager => GlobalDSManager from "cocos/rendering/global-descriptor-set-manager";
    cc::pipeline::GeometryRenderer => GeometryRenderer from "cocos/rendering/geometry-renderer";
    cc::pipeline::PipelineSceneData => PipelineSceneData from "cocos/rendering/pipeline-scene-data";
    ccstd::vector<uint8_t> => Uint8Array;
}
