extern module GfxUniformBlock;
namespace cc::gfx {
    struct UniformBlock;
}

binding typescript {
    UniformBlock => UniformBlock from "cocos/gfx" archive from "cocos/rendering/custom/serialization";
}
