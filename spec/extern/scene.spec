extern module Scene;
namespace cc::scene {
    class Camera;
    class RenderScene;
    class Model;
}

binding typescript {
    Camera => Camera from "cocos/render-scene/scene/camera";
    RenderScene => RenderScene from "cocos/render-scene/core/render-scene";
    Model => Model from "cocos/render-scene/scene";
}
