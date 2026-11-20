extern module Scene;
namespace cc::scene {
    class Camera;
    class RenderScene;
    class Model;
    class DirectionalLight;
    class SphereLight;
    class SpotLight;
    class PointLight;
    class RangedDirectionalLight;
    class SubModel;
}

binding typescript {
    Camera => Camera from "cocos/render-scene/scene/camera";
    RenderScene => RenderScene from "cocos/render-scene/core/render-scene";
    Model => Model from "cocos/render-scene/scene";
}
