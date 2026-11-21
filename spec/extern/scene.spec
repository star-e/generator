extern module Scene;
namespace cc::scene {
    header "cocos/scene/Shadow.h" {
        class Shadows;
    }
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
    DirectionalLight => DirectionalLight from "cocos/render-scene/scene/directional-light";
    SphereLight => SphereLight from "cocos/render-scene/scene/sphere-light";
    SpotLight => SpotLight from "cocos/render-scene/scene/spot-light";
    PointLight => PointLight from "cocos/render-scene/scene/point-light";
    RangedDirectionalLight => RangedDirectionalLight from "cocos/render-scene/scene/ranged-directional-light";
}
