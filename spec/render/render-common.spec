module RenderCommon;
import Light;
import ReflectionProbe;
import Gfx;
import ArchiveInterface;
import Strings;
import Containers;
import EngineTypes;

namespace cc::render {
    enum class UpdateFrequency : uint8_t {
        PER_INSTANCE,
        PER_BATCH,
        PER_PHASE,
        PER_PASS,
        COUNT,
    }

    enum class ParameterType : uint8_t {
        CONSTANTS,
        CBV,
        UAV,
        SRV,
        TABLE,
        SSV,
    }

    @tag struct RasterPassTag {}
    @tag struct RasterSubpassTag {}
    @tag struct ComputeSubpassTag {}
    @tag struct ComputeTag {}
    @tag struct ResolveTag {}
    @tag struct CopyTag {}
    @tag struct MoveTag {}
    @tag struct RaytraceTag {}

    enum class ResourceResidency : uint8_t {
        MANAGED,
        MEMORYLESS,
        PERSISTENT,
        EXTERNAL,
        BACKBUFFER,
        CUSTOM,
    }

    enum class QueueHint : uint8_t {
        NONE,
        OPAQUE,
        MASK,
        BLEND,
        RENDER_OPAQUE = OPAQUE,
        RENDER_CUTOUT = MASK,
        RENDER_TRANSPARENT = BLEND,
    }

    enum class ResourceDimension : uint8_t {
        BUFFER,
        TEXTURE1D,
        TEXTURE2D,
        TEXTURE3D,
    }

    flags ResourceFlags : uint32_t {
        NONE = 0,
        UNIFORM = 0x1,
        INDIRECT = 0x2,
        STORAGE = 0x4,
        SAMPLED = 0x8,
        COLOR_ATTACHMENT = 0x10,
        DEPTH_STENCIL_ATTACHMENT = 0x20,
        INPUT_ATTACHMENT = 0x40,
        SHADING_RATE = 0x80,
        TRANSFER_SRC = 0x100,
        TRANSFER_DST = 0x200,
    }

    @tag struct BufferTag {}
    @tag struct TextureTag {}
    @tag struct Name_ {}
    @tag struct Layout_ {}

    enum class TaskType : uint8_t {
        SYNC,
        ASYNC,
    }

    flags SceneFlags : uint32_t {
        NONE = 0,
        OPAQUE = 0x1,
        MASK = 0x2,
        BLEND = 0x4,
        @deprecated("Please use OPAQUE.")
        OPAQUE_OBJECT = OPAQUE,
        @deprecated("Please use MASK.")
        CUTOUT_OBJECT = MASK,
        @deprecated("Please use BLEND.")
        TRANSPARENT_OBJECT = BLEND,
        SHADOW_CASTER = 0x8,
        @deprecated("Please add 2D node in the render graph.")
        UI = 0x10,
        DEFAULT_LIGHTING = 0x20,
        VOLUMETRIC_LIGHTING = 0x40,
        CLUSTERED_LIGHTING = 0x80,
        PLANAR_SHADOW = 0x100,
        GEOMETRY = 0x200,
        @deprecated("Please add profiler node in the render graph.")
        PROFILER = 0x400,
        DRAW_INSTANCING = 0x800,
        DRAW_NON_INSTANCING = 0x1000,
        REFLECTION_PROBE = 0x2000,
        GPU_DRIVEN = 0x4000,
        NON_BUILTIN = 0x8000,
        ALL = 0xFFFFFFFF,
    }

    enum class LightingMode : uint8_t {
        NONE,
        DEFAULT,
        CLUSTERED,
    }

    enum class AttachmentType : uint8_t {
        RENDER_TARGET,
        DEPTH_STENCIL,
        SHADING_RATE,
    }

    enum class AccessType : uint8_t {
        READ,
        READ_WRITE,
        WRITE,
    }

    enum class ClearValueType : uint8_t {
        NONE,
        FLOAT_TYPE,
        INT_TYPE,
    }

    @serialize struct LightInfo {
        @transient
        nullable light: IntrusivePtr<scene::Light>;
        @transient
        nullable probe: scene::ReflectionProbe* = nullptr;
        level: uint32_t = 0;
        culledByLight: bool = false;

        constructor(light, level, culledByLight, probe) defaults;
    }

    flags ResolveFlags : uint32_t {
        NONE = 0,
        COLOR = 1 << 0,
        DEPTH = 1 << 1,
        STENCIL = 1 << 2,
    }

    @serialize @comparison("==") @hash struct ResolvePair {
        source: ccstd::pmr::string;
        target: ccstd::pmr::string;
        resolveFlags: ResolveFlags = ResolveFlags::NONE;
        mode: gfx::ResolveMode = gfx::ResolveMode::SAMPLE_ZERO;
        mode1: gfx::ResolveMode = gfx::ResolveMode::SAMPLE_ZERO;

        constructor(source, target, resolveFlags, mode, mode1) defaults;
    }

    @serialize struct CopyPair {
        source: ccstd::pmr::string;
        target: ccstd::pmr::string;
        mipLevels: uint32_t = 0xFFFFFFFF;
        numSlices: uint32_t = 0xFFFFFFFF;
        sourceMostDetailedMip: uint32_t = 0;
        sourceFirstSlice: uint32_t = 0;
        sourcePlaneSlice: uint32_t = 0;
        targetMostDetailedMip: uint32_t = 0;
        targetFirstSlice: uint32_t = 0;
        targetPlaneSlice: uint32_t = 0;

        constructor(source, target, mipLevels, numSlices,
                    sourceMostDetailedMip, sourceFirstSlice, sourcePlaneSlice,
                    targetMostDetailedMip, targetFirstSlice, targetPlaneSlice) defaults;
    }

    @moveOnly
    struct UploadPair {
        readonly source: ccstd::vector<uint8_t>;
        target: ccstd::pmr::string;
        mipLevels: uint32_t = 0xFFFFFFFF;
        numSlices: uint32_t = 0xFFFFFFFF;
        targetMostDetailedMip: uint32_t = 0;
        targetFirstSlice: uint32_t = 0;
        targetPlaneSlice: uint32_t = 0;

        constructor(source, target, mipLevels, numSlices,
                    targetMostDetailedMip, targetFirstSlice, targetPlaneSlice) defaults;
    }

    @serialize struct MovePair {
        source: ccstd::pmr::string;
        target: ccstd::pmr::string;
        mipLevels: uint32_t = 0xFFFFFFFF;
        numSlices: uint32_t = 0xFFFFFFFF;
        targetMostDetailedMip: uint32_t = 0;
        targetFirstSlice: uint32_t = 0;
        targetPlaneSlice: uint32_t = 0;

        constructor(source, target, mipLevels, numSlices,
                    targetMostDetailedMip, targetFirstSlice, targetPlaneSlice) defaults;
    }

    @serialize struct PipelineStatistics {
        numRenderPasses: uint32_t = 0;
        numManagedTextures: uint32_t = 0;
        totalManagedTextures: uint32_t = 0;
        numUploadBuffers: uint32_t = 0;
        numUploadBufferViews: uint32_t = 0;
        numFreeUploadBuffers: uint32_t = 0;
        numFreeUploadBufferViews: uint32_t = 0;
        numDescriptorSets: uint32_t = 0;
        numFreeDescriptorSets: uint32_t = 0;
        numInstancingBuffers: uint32_t = 0;
        numInstancingUniformBlocks: uint32_t = 0;
    }
}

binding typescript {
    UpdateFrequency => enum { names; };
    ParameterType => const enum;
    TaskType => const enum;
    LightingMode => const enum;
    AttachmentType => const enum;
    ClearValueType => const enum;
    ResolveFlags => const enum;

    poolModule "cocos/core/memop";
    LightInfo => class { pool li; };
    ResolvePair => class { pool rp; };
    CopyPair => class { pool cp; };
    UploadPair => class { pool up keep(source); };
    MovePair => class { pool mp; };
    PipelineStatistics => class { pool ps; };
}

binding jsb {
    include "cocos/bindings/auto/jsb_gfx_auto.h";
    include "cocos/bindings/auto/jsb_scene_auto.h";
    include "cocos/renderer/pipeline/custom/details/JsbConversion.h";
    record LightInfo;
    record ResolvePair;
    record CopyPair;
    record UploadPair;
    record MovePair;
}
