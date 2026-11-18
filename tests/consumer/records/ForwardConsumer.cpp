#include "cocos/renderer/pipeline/custom/RenderCommonFwd.h"

static_assert(sizeof(cc::render::SceneFlags) == sizeof(uint32_t));
static_assert(sizeof(ccstd::hash<cc::render::ResolvePair>) > 0);
