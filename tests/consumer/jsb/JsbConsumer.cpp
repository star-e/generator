#include "cocos/renderer/pipeline/custom/RenderCommonJsb.h"
#include "cocos/renderer/pipeline/custom/RenderCommonTypes.h"
#include <iostream>

namespace {

bool checkLight(se::ScriptEngine& engine) {
    cc::render::LightInfo light;
    light.level = 3;
    se::Value result, valid;
    if (!nativevalue_to_se(light, result, nullptr) ||
        !engine.getGlobalObject()->setProperty("lightInfo", result) ||
        !engine.evalString("Object.prototype.hasOwnProperty.call(lightInfo, 'light') && "
                           "Object.prototype.hasOwnProperty.call(lightInfo, 'probe') && lightInfo.level === 3",
                           0, &valid) || !valid.isBoolean() || !valid.toBoolean()) return false;

    se::Value input;
    return engine.evalString("({light: null, probe: undefined, culledByLight: true})", 0, &input) &&
           sevalue_to_native(input, &light, nullptr) && light.level == 3 && light.culledByLight;
}

bool checkUpload(se::ScriptEngine& engine) {
    cc::render::UploadPair upload;
    upload.target = "original";
    upload.mipLevels = 7;
    upload.numSlices = 8;
    upload.targetPlaneSlice = 9;

    se::Value input;
    if (!engine.evalString("({source: new Uint8Array([77,0,128,255,33]).subarray(1,4),"
                           "target: 'new target', mipLevels: null, numSlices: undefined})", 0, &input) ||
        !sevalue_to_native(input, &upload, nullptr)) return false;
    if (upload.source != ccstd::vector<uint8_t>{0, 128, 255} || upload.target != "new target" ||
        upload.mipLevels != 7 || upload.numSlices != 8 || upload.targetPlaneSlice != 9) return false;

    se::Value result, valid;
    if (!nativevalue_to_se(upload, result, nullptr) || !engine.getGlobalObject()->setProperty("record", result) ||
        !engine.evalString("record.source instanceof Uint8Array && record.source.join(',') === '0,128,255' && "
                           "record.target === 'new target' && record.mipLevels === 7 && record.targetPlaneSlice === 9",
                           0, &valid) || !valid.isBoolean() || !valid.toBoolean()) return false;

    if (!engine.evalString("({source: null, targetFirstSlice: 17})", 0, &input) ||
        !sevalue_to_native(input, &upload, nullptr) || upload.targetFirstSlice != 17 ||
        upload.source != ccstd::vector<uint8_t>{0, 128, 255}) return false;
    if (!engine.evalString("({source: []})", 0, &input) || !sevalue_to_native(input, &upload, nullptr) ||
        !upload.source.empty() || upload.target != "new target") return false;

    ccstd::vector<cc::render::UploadPair> uploads;
    return engine.evalString("[{target: 'first'}, {target: 'second'}]", 0, &input) &&
           sevalue_to_native(input, &uploads, nullptr) && uploads.size() == 2 &&
           uploads[0].target == "first" && uploads[1].target == "second" &&
           uploads[0].get_allocator().resource() == boost::container::pmr::get_default_resource() &&
           uploads[1].get_allocator().resource() == boost::container::pmr::get_default_resource();
}

} // namespace

int cocos_main(int, const char**) {
    se::ScriptEngine engine;
    if (!engine.init()) return 1;
    {
        se::AutoHandleScope scope;
        if (!checkLight(engine)) {
            std::cerr << "LightInfo JSB conversion failed\n";
            return 1;
        }
        if (!checkUpload(engine)) {
            std::cerr << "UploadPair JSB conversion failed\n";
            return 1;
        }
    }
    return 0;
}

int main() {
    return cocos_main(0, nullptr);
}
