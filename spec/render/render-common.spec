module RenderCommon;
import Light;
import ReflectionProbe;

namespace cc::render {
    struct LightInfo {
        nullable light: IntrusivePtr<scene::Light>;
        nullable probe: scene::ReflectionProbe* = nullptr;
        level: uint32_t = 0;
        culledByLight: bool = false;
    }
}
