extern module GfxUniformBlock;
cpp { header "cocos/renderer/gfx-base/GFXDef-common.h"; }

namespace cc::gfx {
    struct UniformBlock;
}

binding typescript {
    UniformBlock => UniformBlock from "cocos/gfx" archive from "cocos/rendering/custom/serialization";
}
