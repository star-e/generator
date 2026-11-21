#include "bindings/auto/jsb_render_auto.h"
#include <array>
#include <deque>
#include <iostream>

namespace {

template<class Interface>
class TestSetter : public Interface {
public:
    ccstd::string getName() const override {
        ++reads;
        return name;
    }
    void setName(const ccstd::string& value) override {
        ++writes;
        name = value;
    }
    void setCustomBehavior(const ccstd::string&) override {}
    void setMat4(const ccstd::string&, const cc::Mat4&) override {}
    void setQuaternion(const ccstd::string&, const cc::Quaternion&) override {}
    void setColor(const ccstd::string&, const cc::gfx::Color&) override {}
    void setVec4(const ccstd::string&, const cc::Vec4&) override {}
    void setVec2(const ccstd::string&, const cc::Vec2&) override {}
    void setFloat(const ccstd::string&, float) override {}
    void setArrayBuffer(const ccstd::string&, const cc::ArrayBuffer*) override {}
    void setBuffer(const ccstd::string&, cc::gfx::Buffer*) override {}
    void setTexture(const ccstd::string&, cc::gfx::Texture*) override {}
    void setSampler(const ccstd::string&, cc::gfx::Sampler*) override {}
    void setBuiltinCameraConstants(const cc::scene::Camera*) override {}
    void setBuiltinDirectionalLightConstants(const cc::scene::DirectionalLight*, const cc::scene::Camera*) override {}
    void setBuiltinSphereLightConstants(const cc::scene::SphereLight*, const cc::scene::Camera*) override {}
    void setBuiltinSpotLightConstants(const cc::scene::SpotLight*, const cc::scene::Camera*) override {}
    void setBuiltinPointLightConstants(const cc::scene::PointLight*, const cc::scene::Camera*) override {}
    void setBuiltinRangedDirectionalLightConstants(const cc::scene::RangedDirectionalLight*, const cc::scene::Camera*) override {}
    void setBuiltinDirectionalLightFrustumConstants(const cc::scene::Camera*, const cc::scene::DirectionalLight*, uint32_t) override {}
    void setBuiltinSpotLightFrustumConstants(const cc::scene::SpotLight*) override {}

    ccstd::string name;
    mutable unsigned reads{};
    unsigned writes{};
};

struct QueueCall {
    ccstd::string phase;
    ccstd::string pass;
    std::array<uint32_t, 3> groups{};
    uint32_t passID{};
    bool nullMaterial{};
    unsigned dispatches{};
    unsigned destructions{};

    bool matches(const char* expectedPhase, const char* expectedPass,
                 std::array<uint32_t, 3> expectedGroups, uint32_t expectedID) const {
        return phase == expectedPhase && pass == expectedPass && dispatches == 1 &&
               groups == expectedGroups && nullMaterial && passID == expectedID;
    }
};

class TestQueue final : public TestSetter<cc::render::ComputeQueueBuilder> {
public:
    explicit TestQueue(QueueCall& call) : call(call) {}
    ~TestQueue() noexcept override { ++call.destructions; }

    void addDispatch(uint32_t x, uint32_t y, uint32_t z, cc::Material* material, uint32_t passID) override {
        ++call.dispatches;
        call.groups = {x, y, z};
        call.nullMaterial = material == nullptr;
        call.passID = passID;
    }

private:
    QueueCall& call;
};

class TestPass final : public TestSetter<cc::render::ComputePassBuilder> {
public:
    cc::render::ComputeQueueBuilder* addQueue(const ccstd::string& phase, const ccstd::string& pass) override {
        calls.push_back({phase, pass});
        return new TestQueue(calls.back());
    }
    void addTexture(const ccstd::string&, const ccstd::string&, cc::gfx::Sampler*, uint32_t) override {}
    void addStorageBuffer(const ccstd::string&, cc::render::AccessType, const ccstd::string&) override {}
    void addStorageImage(const ccstd::string&, cc::render::AccessType, const ccstd::string&) override {}
    void addMaterialTexture(const ccstd::string&, cc::gfx::ShaderStageFlagBit) override {}
    void setCustomShaderStages(const ccstd::string&, cc::gfx::ShaderStageFlagBit) override {}

    // Queues keep a reference to their call until the JS wrapper finalizes them.
    std::deque<QueueCall> calls;
};

bool checkRenderNode(se::ScriptEngine& engine, TestPass& pass) {
    auto* global = engine.getGlobalObject();
    se::Value object, result;
    return register_all_render(global) &&
           nativevalue_to_se(static_cast<cc::render::ComputePassBuilder*>(&pass), object, nullptr) &&
           global->setProperty("testPass", object) &&
           engine.evalString("testPass.name = 'shadow'; testPass.name", 0, &result) &&
           result.isString() && result.toString() == "shadow" &&
           pass.name == "shadow" && pass.writes == 1 && pass.reads == 1;
}

bool checkQueueCalls(se::ScriptEngine& engine, const TestPass& pass) {
    return engine.evalString(R"(
        var testQueues = [testPass.addQueue(), testPass.addQueue("lighting"),
                          testPass.addQueue("post", "bloom")];
        testQueues[0].addDispatch(1, 2, 3);
        testQueues[1].addDispatch(4, 5, 6, null);
        testQueues[2].addDispatch(7, 8, 9, null, 12);
        testPass = null;
    )") && pass.calls.size() == 3 &&
           pass.calls[0].matches("default", "", {1, 2, 3}, 0) &&
           pass.calls[1].matches("lighting", "", {4, 5, 6}, 0) &&
           pass.calls[2].matches("post", "bloom", {7, 8, 9}, 12);
}

bool checkQueueCollection(se::ScriptEngine& engine, const TestPass& pass) {
    se::AutoHandleScope scope;
    engine.garbageCollect();
    for (const auto& call : pass.calls) {
        if (call.destructions != 0) return false;
    }
    if (!engine.evalString("testQueues = null")) return false;
    engine.garbageCollect();
    for (const auto& call : pass.calls) {
        if (call.destructions != 1) return false;
    }
    return true;
}

} // namespace

int cocos_main(int, const char**) {
    TestPass pass;
    se::ScriptEngine engine;
    if (!engine.init()) return 1;
    {
        se::AutoHandleScope scope;
        if (!checkRenderNode(engine, pass)) {
            std::cerr << "RenderNode.name JSB getter/setter failed\n";
            return 1;
        }
        if (!checkQueueCalls(engine, pass)) {
            std::cerr << "ComputePassBuilder.addQueue or ComputeQueueBuilder.addDispatch arguments differ\n";
            return 1;
        }
    }
    if (!checkQueueCollection(engine, pass)) {
        std::cerr << "ComputeQueueBuilder GC lifetime differs\n";
        return 1;
    }
    std::cout << "RenderNode.name = shadow (1 native write, 1 native read)\n";
    std::cout << "addQueue(0/1/2 arguments), addDispatch(3/4/5 arguments), 3 queues collected once each\n";
    return 0;
}

int main() {
    return cocos_main(0, nullptr);
}
