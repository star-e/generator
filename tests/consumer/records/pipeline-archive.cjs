const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const flags = require('./platform.cjs');
const [jsDir, nativeFile, webFile, legacyFile, convertedFile, printFile] = process.argv.slice(2);
const current = require(path.join(jsDir, 'layout-graph.js'));
const legacy = require(path.join(jsDir, 'legacy-layout.js'));
const { BinaryInputArchive, BinaryOutputArchive } = require(path.join(jsDir, 'binary-archive.js'));
const { UpdateFrequency } = require(path.join(jsDir, 'types.js'));
const { DescriptorSetLayoutBinding, DescriptorType, ShaderStageFlagBit } = require(path.resolve(jsDir, '../../gfx'));
const pass = UpdateFrequency.PER_PASS, phase = UpdateFrequency.PER_PHASE;
const { COMPUTE } = ShaderStageFlagBit;
const roots = ['DescriptorSetData', 'ShaderProgramData', 'EffectData', 'RenderStageData', 'RenderPhaseData'];

function descriptorSet(api, slot) {
    const value = new api.DescriptorSetData();
    value.descriptorSetLayoutData.slot = slot;
    value.descriptorSetLayoutData.bindingMap.set(42, slot + 1);
    value.descriptorSetLayoutInfo.bindings.push(new DescriptorSetLayoutBinding(slot, DescriptorType.UNIFORM_BUFFER, 2, COMPUTE));
    return value;
}

function program(api) {
    const value = new api.ShaderProgramData();
    value.layout.descriptorSets.set(pass, descriptorSet(api, 3));
    value.layout.descriptorGroups.set(pass, descriptorSet(api, 9));
    return value;
}

function records(api) {
    const shader = new api.ShaderLayoutData();
    const bindings = new api.ShaderBindingData(), layout = new api.DescriptorSetLayoutData();
    bindings.descriptorBindings.set(42, 23);
    layout.bindingMap.set(42, 12);
    shader.bindingData.set(phase, bindings);
    shader.layoutData.set(pass, layout);
    const effect = new api.EffectData(), technique = new api.TechniqueData();
    technique.passes.push(shader);
    effect.techniques.set('lighting', technique);
    const stage = new api.RenderStageData(), renderPhase = new api.RenderPhaseData();
    stage.descriptorVisibility.set(42, COMPUTE);
    renderPhase.rootSignature = 'main';
    renderPhase.shaderPrograms.push(program(api));
    renderPhase.shaderIndex.set('bloom', 0);
    return [descriptorSet(api, 7), program(api), effect, stage, renderPhase];
}

function attachHandles([set, shader, , , renderPhase], handles) {
    [set.descriptorSetLayout, set.descriptorSet, shader.pipelineLayout, renderPhase.pipelineLayout] = handles;
}

function graphData(api, [set, , effect, stage, renderPhase]) {
    const graph = new api.LayoutGraphData();
    const layout = new api.PipelineLayoutData();
    layout.descriptorSets.set(pass, set);
    const root = graph.addVertex(0, stage, 'main', pass, layout);
    const child = graph.addVertex(1, renderPhase, 'lighting', phase, new api.PipelineLayoutData(), root);
    graph.valueNames.push('camera', 'bloom');
    graph.attributeIndex.set('camera', 42);
    graph.constantIndex.set('tint', 12);
    graph.shaderLayoutIndex.set('bloom', child);
    graph.effects.set('post', effect);
    graph.constantMacros = 'writer macros';
    return graph;
}

function graphValues(g) {
    return [Array.from(g.v(), v => [g.w(v), g.getParent(v), g.getName(v), g.getUpdate(v), g.getLayout(v), g.value(g.w(v), v)]),
        g.valueNames, g.attributeIndex, g.constantIndex, g.shaderLayoutIndex, g.effects];
}

function encode(api) {
    const values = records(api), archive = new BinaryOutputArchive();
    attachHandles(values, [{}, {}, {}, {}]);
    roots.forEach((type, i) => api['save' + type](archive, values[i]));
    api.saveLayoutGraphData(archive, new api.LayoutGraphData());
    api.saveLayoutGraphData(archive, graphData(api, values));
    return Buffer.from(archive.data);
}

function checkArchive(api, bytes) {
    const archive = new BinaryInputArchive(bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.length));
    const values = roots.map(type => new api[type]());
    const handles = [{}, {}, {}, {}];
    attachHandles(values, handles);
    roots.forEach((type, i) => api['load' + type](archive, values[i]));
    const [set, shader, , , renderPhase] = values;
    assert.strictEqual(set.descriptorSetLayout, handles[0]);
    assert.strictEqual(set.descriptorSet, handles[1]);
    assert.strictEqual(shader.pipelineLayout, handles[2]);
    assert.strictEqual(renderPhase.pipelineLayout, handles[3]);
    const expected = records(api);
    attachHandles(expected, handles);
    assert.deepEqual(values, expected);
    const graph = new api.LayoutGraphData();
    graph.constantMacros = 'reader macros';
    const names = graph.valueNames, effects = graph.effects;
    api.loadLayoutGraphData(archive, graph);
    assert.equal(graph.nv(), 0);
    const graphOffset = archive.offset;
    api.loadLayoutGraphData(archive, graph);
    assert.deepEqual(graphValues(graph), graphValues(graphData(api, records(api))));
    assert.equal(graph.constantMacros, 'reader macros');
    assert.strictEqual(graph.valueNames, names);
    assert.strictEqual(graph.effects, effects);
    if (api === current) {
        const truncated = bytes.subarray(graphOffset, bytes.length - 1);
        const input = new BinaryInputArchive(truncated.buffer.slice(truncated.byteOffset, truncated.byteOffset + truncated.length));
        const effect = graph.effects.get('post');
        graph.valueNames.push('keep');
        assert.throws(() => api.loadLayoutGraphData(input, graph));
        assert.deepEqual(graph.valueNames, ['camera', 'bloom', 'keep']);
        assert.equal(graph.nv(), 2);
        assert.strictEqual(graph.effects.get('post'), effect);
    }
    assert.equal(archive.offset, bytes.length);
}

const native = fs.readFileSync(nativeFile), web = encode(current), old = encode(legacy);
assert.deepEqual(native, old);
assert.deepEqual(web, old);
for (const bytes of [native, web, old]) {
    checkArchive(current, bytes);
    checkArchive(legacy, bytes);
}
fs.writeFileSync(webFile, web);
fs.writeFileSync(legacyFile, old);

const editor = require(path.join(jsDir, 'layout-graph-conversion.js'));
const oldEditor = require(path.join(jsDir, 'legacy-layout-conversion.js'));
const input = oldEditor.makeInput();
assert.deepEqual(Buffer.from(editor.makeInput()), Buffer.from(input), 'Editor input archives differ');
const converted = Buffer.from(editor.convert(input));
assert.deepEqual(converted, Buffer.from(oldEditor.convert(input)), 'Converted LayoutGraphData archives differ');
fs.writeFileSync(convertedFile, converted);
const printSample = Buffer.from(editor.makePrintSample());
assert.deepEqual(printSample, Buffer.from(oldEditor.makePrintSample()), 'Native printer input archives differ');
fs.writeFileSync(printFile, printSample);

const [set, shader, effect, stage, renderPhase] = records(current);
const { descriptorSetLayoutData, descriptorSetLayoutInfo } = set;
const { bindings } = descriptorSetLayoutInfo;
const layoutHandle = {}, setHandle = {};
const constructed = new current.DescriptorSetData(descriptorSetLayoutData, layoutHandle, setHandle);
assert.strictEqual(constructed.descriptorSetLayoutData, descriptorSetLayoutData);
assert.strictEqual(constructed.descriptorSetLayout, layoutHandle);
assert.strictEqual(constructed.descriptorSet, setHandle);
set.reset(layoutHandle, setHandle);
assert.strictEqual(set.descriptorSetLayoutData, descriptorSetLayoutData);
assert.strictEqual(set.descriptorSetLayoutInfo, descriptorSetLayoutInfo);
assert.strictEqual(descriptorSetLayoutInfo.bindings, bindings);
assert.strictEqual(set.descriptorSetLayout, layoutHandle);
assert.strictEqual(set.descriptorSet, setHandle);
assert.equal(bindings.length, 0);
assert.deepEqual(descriptorSetLayoutData, new current.DescriptorSetLayoutData());

const { layout } = shader;
const { descriptorSets, descriptorGroups } = layout;
assert.equal(current.Layout.isWebGPU, false);
assert.equal(current.Layout.type, legacy.Layout.type);
for (const runtime of [false, true]) for (const html5 of [false, true]) for (const webgpu of [false, true]) {
    flags.COCOS_RUNTIME = runtime;
    flags.HTML5 = html5;
    current.Layout.isWebGPU = webgpu;
    const selected = (runtime || html5) && webgpu ? descriptorGroups : descriptorSets;
    assert.strictEqual(layout.getSets(), selected);
    assert.strictEqual(layout.getSet(pass), selected.get(pass));
    assert.equal(layout.getSet(phase), undefined);
}
current.Layout.isWebGPU = false;
shader.pipelineLayout = {};
shader.reset();
assert.strictEqual(shader.layout, layout);
assert.strictEqual(layout.descriptorSets, descriptorSets);
assert.strictEqual(layout.descriptorGroups, descriptorGroups);
assert.deepEqual(shader, new current.ShaderProgramData());

const technique = effect.techniques.get('lighting'), passData = technique.passes[0];
const binding = passData.bindingData.get(phase);
for (const [type, instance, fields] of [
    ['ShaderBindingData', binding, ['descriptorBindings']],
    ['ShaderLayoutData', passData, ['layoutData', 'bindingData']],
    ['TechniqueData', technique, ['passes']],
    ['EffectData', effect, ['techniques']],
    ['RenderStageData', stage, ['descriptorVisibility']],
    ['RenderPhaseData', renderPhase, ['shaderPrograms', 'shaderIndex']],
]) {
    const refs = fields.map(field => instance[field]);
    if (instance === renderPhase) instance.pipelineLayout = {};
    instance.reset();
    assert.deepEqual(instance, new current[type]());
    fields.forEach((field, i) => assert.strictEqual(instance[field], refs[i]));
}
