extern module RenderWindow;
cpp { header "cocos/scene/RenderWindow.h"; }

namespace cc::scene {
    class RenderWindow;
}

binding typescript {
    RenderWindow => RenderWindow from "cocos/render-scene/core/render-window";
}
