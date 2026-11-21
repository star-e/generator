extern module Scene;
namespace cc::scene {
    header "cocos/scene/{name}.h" {
        class Camera;
        class RenderScene;
        class Model;
        class DirectionalLight;
        class SphereLight;
        class SpotLight;
        class PointLight;
        class RangedDirectionalLight;
        class SubModel;
        class Pass;
    }
}

binding typescript {
    Camera => Camera from "cocos/render-scene/scene/camera";
    RenderScene => RenderScene from "cocos/render-scene/core/render-scene";
    Model => Model from "cocos/render-scene/scene";
}
