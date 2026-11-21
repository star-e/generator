#include "cocos/renderer/pipeline/custom/NativeFwd.h"
#include <type_traits>

static_assert(std::is_class_v<cc::render::ProgramLibrary>);
static_assert(std::is_class_v<cc::render::RenderingModule>);
static_assert(std::is_class_v<cc::render::InputArchive>);
static_assert(sizeof(cc::render::SceneFlags) == sizeof(uint32_t));
static_assert(sizeof(ccstd::hash<cc::render::ResolvePair>) > 0);
static_assert(sizeof(ccstd::hash<cc::render::NameLocalID>) > 0);

#include "ShiftProbeFwd.h"
static_assert(std::is_same_v<cc::LocalUniformID, uint32_t>);
