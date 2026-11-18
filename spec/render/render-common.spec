module RenderCommon;
import Light;
import ReflectionProbe;

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

    enum class TaskType : uint8_t {
        SYNC,
        ASYNC,
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

    struct LightInfo {
        nullable light: IntrusivePtr<scene::Light>;
        nullable probe: scene::ReflectionProbe* = nullptr;
        level: uint32_t = 0;
        culledByLight: bool = false;

        constructor(light, level, culledByLight, probe);
        constructor(light, level);
    }
}
