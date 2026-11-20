extern module PipelineExternals;
namespace cc {
    class MacroRecord;
    class RefCounted;
    class EffectAsset;
    struct IProgramInfo;
    class ArrayBuffer;
    class Mat4;
    class Quaternion;
    class Vec2;
    class Vec4;
}

binding typescript {
    EffectAsset => EffectAsset from "cocos/asset/assets";
    MacroRecord => MacroRecord from "cocos/render-scene/core/pass-utils";
    IProgramInfo => IProgramInfo from "cocos/render-scene/core/program-lib";
}
