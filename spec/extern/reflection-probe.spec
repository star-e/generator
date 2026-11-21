extern module ReflectionProbe;
cpp { header "cocos/scene/ReflectionProbe.h"; }

namespace cc::scene {
    class ReflectionProbe;
}

binding typescript {
    ReflectionProbe => ReflectionProbe from "cocos/render-scene/scene/reflection-probe";
}
