const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const flags = require('./platform.cjs');
const [jsDir, convertedFile] = process.argv.slice(2);
const loadLayoutArchive = require('./layout-archive-api.cjs');
const { UpdateFrequency } = require(path.join(jsDir, 'types.js'));
const { DescriptorSetInfo, DescriptorSetLayoutBinding, DescriptorSetLayoutInfo, DescriptorType, Format,
    MemoryAccessBit, SampleType, ShaderStageFlagBit, ViewDimension } = require(path.resolve(jsDir, '../../gfx'));
const file = fs.readFileSync(convertedFile);
const bytes = file.buffer.slice(file.byteOffset, file.byteOffset + file.byteLength);

const { PER_PASS, PER_PHASE, PER_INSTANCE, PER_BATCH } = UpdateFrequency;
const { UNIFORM_BUFFER, SAMPLER_TEXTURE, STORAGE_IMAGE, TEXTURE } = DescriptorType;
const fragment = ShaderStageFlagBit.FRAGMENT;
const camera = new DescriptorSetLayoutBinding(0, UNIFORM_BUFFER, 1, fragment);
const ordinary = [
    [1, PER_PASS, [camera, new DescriptorSetLayoutBinding(3, SAMPLER_TEXTURE, 1, fragment),
        new DescriptorSetLayoutBinding(4, SAMPLER_TEXTURE, 2, fragment)]],
    [2, PER_INSTANCE, []],
    [2, PER_PHASE, [new DescriptorSetLayoutBinding(0, TEXTURE, 1, fragment)]],
];
const groups = [
    [1, PER_PASS, [camera, new DescriptorSetLayoutBinding(3, STORAGE_IMAGE, 1, fragment, MemoryAccessBit.READ_WRITE,
        ViewDimension.TEX2D_ARRAY, SampleType.UINT, Format.RGBA8UI)]],
    [2, PER_BATCH, []],
];

function checkBindings(api, utils, webGPU, withDevice) {
    api.Layout.isWebGPU = webGPU;
    const graph = new api.LayoutGraphData();
    api.loadLayoutGraphData(new api.BinaryInputArchive(bytes), graph);
    const device = {
        createDescriptorSetLayout(info) {
            assert.ok(info instanceof DescriptorSetLayoutInfo);
            return { info };
        },
        createDescriptorSet(info) {
            assert.ok(info instanceof DescriptorSetInfo);
            return { layout: info.layout };
        },
    };
    utils.createGfxDescriptorSetsAndPipelines(withDevice ? device : null, graph);
    const result = [];
    for (const v of graph.v()) {
        const { descriptorSets, descriptorGroups } = graph.getLayout(v);
        const selected = webGPU ? descriptorGroups : descriptorSets;
        const other = webGPU ? descriptorSets : descriptorGroups;
        for (const data of other.values()) {
            assert.equal(data.descriptorSet, null);
            assert.equal(data.descriptorSetLayout, null);
            assert.deepEqual(data.descriptorSetLayoutInfo.bindings, []);
        }
        for (const [rate, data] of selected) {
            if (withDevice) {
                assert.ok(data.descriptorSetLayout);
                assert.strictEqual(data.descriptorSet.layout, data.descriptorSetLayout);
                result.push([v, rate, data.descriptorSetLayout.info.bindings]);
            } else {
                assert.equal(data.descriptorSetLayout, null);
                assert.equal(data.descriptorSet, null);
                result.push([v, rate, data.descriptorSetLayoutInfo.bindings]);
            }
        }
    }
    assert.deepEqual(result, webGPU ? groups : ordinary);
    return result;
}

function exercise(directory) {
    const api = loadLayoutArchive(directory);
    const utils = require(path.join(directory, 'layout-graph-conversion.js'));
    const graph = new api.LayoutGraphData();
    assert.equal(utils.printLayoutGraphData(graph), '');
    api.loadLayoutGraphData(new api.BinaryInputArchive(bytes), graph);
    const printed = utils.printLayoutGraphData(graph);
    assert.equal(new utils.LayoutGraphBuilder2(graph).print(), printed);
    assert.deepEqual(printed.split('\n').filter(line => /: Render(Stage|Phase) \{$/.test(line)), [
        '"scene": RenderStage {', '    "light": RenderStage {',
        '        "opaque": RenderPhase {', '        "transparent": RenderPhase {',
    ]);
    for (const text of ['UniformBlock "camera"', 'exposure[2]: Float4', 'Block<SAMPLER_TEXTURE, Fragment>', '"shadowB[2]"']) {
        assert.ok(printed.includes(text), text);
    }
    const bindings = [];
    for (const webGPU of [false, true]) {
        for (const withDevice of [false, true]) bindings.push(checkBindings(api, utils, webGPU, withDevice));
    }
    return { printed, bindings };
}

flags.HTML5 = true;
assert.deepEqual(exercise(jsDir), exercise(path.join(jsDir, '../legacy')));
