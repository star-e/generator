#include "cocos/renderer/pipeline/custom/RenderCommonTypes.h"

#include <cstddef>
#include <type_traits>

using cc::render::AccessType;
using cc::render::LightInfo;

static_assert(std::is_same_v<std::underlying_type_t<AccessType>, uint8_t>);
static_assert(static_cast<int>(AccessType::READ) == 0 && static_cast<int>(AccessType::READ_WRITE) == 1 &&
              static_cast<int>(AccessType::WRITE) == 2);
static_assert(std::is_same_v<decltype(LightInfo::light), cc::IntrusivePtr<cc::scene::Light>>);
static_assert(std::is_same_v<decltype(LightInfo::probe), cc::scene::ReflectionProbe*>);

int main() {
    LightInfo empty;
    if (empty.light.get() || empty.probe || empty.level != 0 || empty.culledByLight) return 1;

    LightInfo shortForm(nullptr, 7);
    if (shortForm.light.get() || shortForm.probe || shortForm.level != 7 || shortForm.culledByLight) return 2;

    // The generated type stores an opaque probe pointer and never dereferences it.
    alignas(std::max_align_t) std::byte token{};
    auto* probe = reinterpret_cast<cc::scene::ReflectionProbe*>(&token);
    LightInfo full(nullptr, 42, true, probe);
    return full.light.get() || full.probe != probe || full.level != 42 || !full.culledByLight;
}
