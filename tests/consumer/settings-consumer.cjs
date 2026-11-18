const assert = require('assert').strict;
const path = require('path');
const [root, asset] = process.argv.slice(2);
const { Camera, Material, Texture2D, gfx, registeredEnums, rendering } = require(path.join(root, 'node_modules/cc'));
const api = require(path.join(root, asset, 'builtin-pipeline-types'));
const { BuiltinPipelineSettings } = require(path.join(root, asset, 'builtin-pipeline-settings'));

assert.deepEqual(registeredEnums, [api.BloomType]);
assert.equal(api.BloomType.MipmapFilter, 1);
assert.equal(api.makeForwardPassConfigs, undefined);
assert.equal(api.fillRequiredForwardPassConfigs, undefined);
assert.deepEqual(api.makeHBAO(), {
    enabled: false, radiusScale: 1, angleBiasDegree: 10, blurSharpness: 3, aoSaturation: 1, needBlur: false,
});

const defaults = api.makePipelineSettings();
assert.equal(defaults.msaa.sampleCount, gfx.SampleCount.X4);
assert.equal(defaults.bloom.type, api.BloomType.KawaseDualFilter);
assert.equal(defaults.bloom.iterations, 3);
assert.equal(defaults.bloom.threshold, 0.8);
assert.equal(defaults.shadingScale, 0.5);
const other = api.makePipelineSettings();
assert.deepEqual(other, defaults);
for (const key of ['msaa', 'bloom', 'toneMapping', 'colorGrading', 'fsr', 'fxaa']) {
    assert.notEqual(defaults[key], other[key]);
}

const legacy = new Material();
const replacement = new Material();
const texture = new Texture2D();
for (const [serialized, expected] of [
    [{ material: legacy }, legacy],
    [{ material: legacy, kawaseFilterMaterial: undefined }, legacy],
    [{ material: legacy, kawaseFilterMaterial: replacement }, replacement],
    [{ material: legacy, kawaseFilterMaterial: null }, null],
    [{}, null],
]) {
    const component = new BuiltinPipelineSettings();
    const bloom = { ...serialized, enabled: false, threshold: 0, custom: 42 };
    const settings = { bloom, msaa: null, shadingScale: 0, colorGrading: { colorGradingMap: texture } };
    component._settings = settings; // Simulate editor deserialization before the lifecycle callback.
    component.editorPreview = true;
    const camera = component.getComponent(Camera).camera;
    for (let i = 0; i < 2; ++i) {
        component.onEnable();
        assert.equal(component.getPipelineSettings(), settings);
        assert.equal(camera.pipelineSettings, settings);
        assert.equal(rendering.getEditorPipelineSettings(), settings);
        assert.equal(settings.bloom, bloom);
        assert.equal(component.kawaseBloomMaterial, expected);
        assert.equal(bloom.material, serialized.material || null);
        assert.equal(bloom.mipmapFilterMaterial, null);
        assert.equal(bloom.threshold, 0);
        assert.equal(bloom.custom, 42);
        assert.equal(settings.shadingScale, 0);
        assert.equal(settings.colorGrading.colorGradingMap, texture);
        assert.deepEqual(settings.msaa, defaults.msaa);
        for (const key of ['toneMapping', 'fsr', 'fxaa']) assert.deepEqual(settings[key], defaults[key]);
        component.onDisable();
        assert.equal(camera.pipelineSettings, null);
        assert.equal(rendering.getEditorPipelineSettings(), null);
    }
}
