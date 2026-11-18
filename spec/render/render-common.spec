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

    enum class AccessType : uint8_t {
        READ,
        READ_WRITE,
        WRITE,
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
