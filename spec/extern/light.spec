extern module Light;
cpp { header "cocos/scene/Light.h"; }

namespace cc::scene {
    class Light;
}

binding typescript {
    Light => Light from "cocos/render-scene/scene";
}
