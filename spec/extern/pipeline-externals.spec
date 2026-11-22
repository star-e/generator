extern module PipelineExternals;
namespace cc {
    header "cocos/renderer/core/PassUtils.h" {
        type MacroRecord;
    }

    header "cocos/base/RefCounted.h" {
        class RefCounted {
        public:
            virtual ~RefCounted();
        };
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
    ArrayBuffer => "ArrayBuffer";
    Mat4 => Mat4 from "cocos/core/math";
    Quaternion => Quat from "cocos/core/math";
    Vec2 => Vec2 from "cocos/core/math";
    Vec4 => Vec4 from "cocos/core/math";
    EffectAsset => EffectAsset from "cocos/asset/assets";
    MacroRecord => MacroRecord from "cocos/render-scene/core/pass-utils";
    IProgramInfo => extern interface IProgramInfo from "cocos/render-scene/core/program-lib";
}
