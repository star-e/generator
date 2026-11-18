#include "cocos/renderer/pipeline/custom/RenderCommonTypes.h"

#include <cstddef>
#include <initializer_list>
#include <type_traits>

using cc::render::AccessType;
using cc::render::AttachmentType;
using cc::render::ClearValueType;
using cc::render::LightInfo;
using cc::render::LightingMode;
using cc::render::ParameterType;
using cc::render::QueueHint;
using cc::render::ResourceDimension;
using cc::render::ResourceFlags;
using cc::render::ResourceResidency;
using cc::render::TaskType;
using cc::render::UpdateFrequency;

template<class Enum>
constexpr bool hasSequentialValues(std::initializer_list<Enum> members) {
    static_assert(std::is_same_v<std::underlying_type_t<Enum>, uint8_t>);
    unsigned expected = 0;
    for (auto member : members) {
        if (static_cast<unsigned>(member) != expected++) return false;
    }
    return true;
}

static_assert(hasSequentialValues({UpdateFrequency::PER_INSTANCE, UpdateFrequency::PER_BATCH, UpdateFrequency::PER_PHASE,
                                   UpdateFrequency::PER_PASS, UpdateFrequency::COUNT}));
static_assert(hasSequentialValues({ParameterType::CONSTANTS, ParameterType::CBV, ParameterType::UAV,
                                   ParameterType::SRV, ParameterType::TABLE, ParameterType::SSV}));
static_assert(hasSequentialValues({ResourceResidency::MANAGED, ResourceResidency::MEMORYLESS, ResourceResidency::PERSISTENT,
                                   ResourceResidency::EXTERNAL, ResourceResidency::BACKBUFFER, ResourceResidency::CUSTOM}));
static_assert(hasSequentialValues({QueueHint::NONE, QueueHint::OPAQUE, QueueHint::MASK, QueueHint::BLEND}));
static_assert(QueueHint::RENDER_OPAQUE == QueueHint::OPAQUE);
static_assert(QueueHint::RENDER_CUTOUT == QueueHint::MASK);
static_assert(QueueHint::RENDER_TRANSPARENT == QueueHint::BLEND);
static_assert(hasSequentialValues({ResourceDimension::BUFFER, ResourceDimension::TEXTURE1D,
                                   ResourceDimension::TEXTURE2D, ResourceDimension::TEXTURE3D}));
static_assert(hasSequentialValues({TaskType::SYNC, TaskType::ASYNC}));
static_assert(hasSequentialValues({LightingMode::NONE, LightingMode::DEFAULT, LightingMode::CLUSTERED}));
static_assert(hasSequentialValues({AttachmentType::RENDER_TARGET, AttachmentType::DEPTH_STENCIL, AttachmentType::SHADING_RATE}));
static_assert(hasSequentialValues({AccessType::READ, AccessType::READ_WRITE, AccessType::WRITE}));
static_assert(hasSequentialValues({ClearValueType::NONE, ClearValueType::FLOAT_TYPE, ClearValueType::INT_TYPE}));
static_assert(std::is_same_v<std::underlying_type_t<ResourceFlags>, uint32_t>);
constexpr bool checkResourceFlags() {
    auto usage = ResourceFlags::COLOR_ATTACHMENT | ResourceFlags::SAMPLED;
    if (static_cast<uint32_t>(usage) != 0x18 || !any(usage & ResourceFlags::SAMPLED)) return false;
    if (any(usage & ResourceFlags::STORAGE) || !usage) return false;
    static_assert(!ResourceFlags::NONE && !any(ResourceFlags::NONE));
    if (&(usage |= ResourceFlags::STORAGE) != &usage || static_cast<uint32_t>(usage) != 0x1C) return false;
    if (&(usage &= ~ResourceFlags::SAMPLED) != &usage || static_cast<uint32_t>(usage) != 0x14) return false;
    return static_cast<uint32_t>(~ResourceFlags::NONE) == UINT32_MAX;
}
static_assert(checkResourceFlags());
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
