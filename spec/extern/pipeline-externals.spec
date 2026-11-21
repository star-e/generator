extern module PipelineExternals;
namespace cc {
    header "cocos/renderer/core/PassUtils.h" {
        class MacroRecord;
    }

    header "cocos/base/RefCounted.h" {
        class RefCounted;
    }

    header "cocos/core/assets/EffectAsset.h" {
        class EffectAsset;
    }

    header "cocos/renderer/core/ProgramLib.h" {
        struct IProgramInfo;
    }

    header "cocos/core/ArrayBuffer.h" {
        class ArrayBuffer;
    }

    header "cocos/math/{name}.h" {
        class Mat4;
        class Quaternion;
        class Vec2;
        class Vec4;
    }
}

binding typescript {
    EffectAsset => EffectAsset from "cocos/asset/assets";
    MacroRecord => MacroRecord from "cocos/render-scene/core/pass-utils";
    IProgramInfo => IProgramInfo from "cocos/render-scene/core/program-lib";
}
