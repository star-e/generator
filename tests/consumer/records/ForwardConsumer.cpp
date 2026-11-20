#include "cocos/renderer/pipeline/custom/RenderCommonFwd.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphFwd.h"
#include "ShiftProbeFwd.h"
#include <type_traits>

static_assert(sizeof(cc::render::SceneFlags) == sizeof(uint32_t));
static_assert(sizeof(ccstd::hash<cc::render::ResolvePair>) > 0);
static_assert(sizeof(ccstd::hash<cc::render::NameLocalID>) > 0);
static_assert(std::is_same_v<cc::LocalUniformID, uint32_t>);
