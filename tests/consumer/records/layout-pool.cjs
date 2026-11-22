const assert = require('assert').strict;
const path = require('path');
require('./platform.cjs');
const jsDir = process.argv[2];
const current = require(path.join(jsDir, 'layout-graph.js'));
const legacy = require(path.join(jsDir, '../legacy/layout-graph.js'));
const { RenderCommonObjectPool, UpdateFrequency } = require(path.join(jsDir, 'types.js'));
const { descriptorBlockArgs, descriptorGroupBlockArgs, descriptorBlockDataArgs } = require(path.join(jsDir, 'enum-consumer.js'));
const { DescriptorSetLayoutBinding, Type } = require(path.resolve(jsDir, '../../gfx'));
const layoutHandle = { id: 'layout' }, setHandle = { id: 'set' };
const pass = UpdateFrequency.PER_PASS;

// Each sample leaves live data in both checked-out objects before returning them to the pool.
const cases = [
    ['Layout', [], () => {}],
    ['Descriptor', [Type.FLOAT4], (v, i) => { v.count = i; }],
    ['DescriptorBlock', [], (v, i, api) => { v.capacity = i; v.descriptors.set('d', new api.Descriptor(Type.FLOAT4)); }],
    ['DescriptorBlockFlattened', [], (v, i) => { v.count = i; v.descriptorNames.push('d' + i); }],
    ['DescriptorBlockIndex', descriptorBlockArgs, (v, i) => { v.visibility = i; }],
    ['DescriptorGroupBlockIndex', descriptorGroupBlockArgs, (v, i) => { v.format = i; }],
    ['DescriptorDB', [], (v, i, api) => { v.blocks.set('d' + i, new api.DescriptorBlock()); }],
    ['RenderPhase', [], (v, i) => { v.shaders.add('shader' + i); }],
    ['LayoutGraph', [], (v, i, api) => { v.addVertex(0, 0, 'stage' + i, new api.DescriptorDB()); }],
    ['UniformData', [31, Type.FLOAT4, 24], (v, i) => { v.size = i; }],
    ['UniformBlockData', [], (v, i, api) => { v.bufferSize = i; v.uniforms.push(new api.UniformData()); }],
    ['DescriptorData', [17, Type.SAMPLER2D, 4], (v, i) => { v.count = i; }],
    ['DescriptorBlockData', descriptorBlockDataArgs, (v, i, api) => { v.offset = i; v.descriptors.push(new api.DescriptorData()); }],
    ['DescriptorSetLayoutData', [3, 11], (v, i) => { v.uniformBlockCapacity = i; v.bindingMap.set(1, i); }],
    ['DescriptorSetData', [layoutHandle, setHandle], (v, i) => {
        v.descriptorSetLayoutData.bindingMap.set(1, i);
        v.descriptorSetLayoutInfo.bindings.push(new DescriptorSetLayoutBinding(i));
    }],
    ['PipelineLayoutData', [], (v, i, api) => { v.descriptorGroups.set(pass, new api.DescriptorSetData()); }],
    ['ShaderBindingData', [], (v, i) => { v.descriptorBindings.set(1, i); }],
    ['ShaderLayoutData', [], (v, i, api) => { v.bindingData.set(pass, new api.ShaderBindingData()); }],
    ['TechniqueData', [], (v, i, api) => { v.passes.push(new api.ShaderLayoutData()); }],
    ['EffectData', [], (v, i, api) => { v.techniques.set('t' + i, new api.TechniqueData()); }],
    ['ShaderProgramData', [], (v, i, api) => { v.pipelineLayout = layoutHandle; v.layout.descriptorSets.set(pass, new api.DescriptorSetData()); }],
    ['RenderStageData', [], (v, i) => { v.descriptorVisibility.set(1, i); }],
    ['RenderPhaseData', [], (v, i) => { v.rootSignature = 'root' + i; v.shaderIndex.set('shader', i); v.pipelineLayout = layoutHandle; }],
    ['LayoutGraphData', [], (v, i, api) => {
        v.addVertex(0, new api.RenderStageData(), 'stage' + i, pass, new api.PipelineLayoutData());
        v.attributeIndex.set('descriptor', i);
        v.constantIndex.set('constant', i);
        v.shaderLayoutIndex.set('shader', 0);
        v.effects.set('effect', new api.EffectData());
        v.valueNames.push('name' + i);
        v.constantMacros = 'old macros';
    }],
];

function snapshot(name, value) {
    if (name === 'LayoutGraph' || name === 'LayoutGraphData') {
        const vertices = Array.from(value.v(), id => value.getName(id));
        if (name === 'LayoutGraph') return { vertices };
        return { vertices, attributes: Array.from(value.attributeIndex), constants: Array.from(value.constantIndex),
            shaders: Array.from(value.shaderLayoutIndex), effects: Array.from(value.effects.keys()),
            valueNames: Array.from(value.valueNames), macros: value.constantMacros };
    }
    // Compare public data across the two sets of classes, including collection contents.
    return JSON.parse(JSON.stringify(value, (_, field) => field instanceof Map || field instanceof Set ? Array.from(field) : field));
}

function exercise(api) {
    const common = new RenderCommonObjectPool();
    const reserved = common.createPipelineStatistics();
    reserved.numRenderPasses = 71;
    const pool = new api.LayoutGraphObjectPool(common);
    assert.strictEqual(pool.renderCommon, common);
    const factories = Object.getOwnPropertyNames(api.LayoutGraphObjectPool.prototype).filter(name => name.startsWith('create'));
    assert.deepEqual(factories.sort(), cases.map(([name]) => 'create' + name).sort());

    const samples = cases.map(([name, args, dirty]) => {
        const create = (...params) => pool['create' + name](...params);
        const first = create(...args);
        const second = create();
        assert.ok(first instanceof api[name] && second instanceof api[name], name);
        assert.notStrictEqual(first, second, name);
        const explicit = snapshot(name, first), defaults = snapshot(name, second);
        dirty(first, 7, api);
        dirty(second, 19, api);
        return { name, args, create, first, second, explicit, defaults, dirtyFirst: snapshot(name, first), dirtySecond: snapshot(name, second) };
    });
    pool.reset();
    assert.notStrictEqual(common.createPipelineStatistics(), reserved);
    assert.equal(reserved.numRenderPasses, 71);
    for (const { name, args, create, first, second, explicit, defaults, dirtyFirst, dirtySecond } of samples) {
        assert.deepEqual(snapshot(name, first), dirtyFirst, name + ' changed during pool.reset()');
        assert.strictEqual(create(), first, name);
        assert.deepEqual(snapshot(name, first), defaults, name + ' default reuse');
        assert.deepEqual(snapshot(name, second), dirtySecond, name + ' changed before checkout');
        assert.strictEqual(create(...args), second, name);
        assert.deepEqual(snapshot(name, second), explicit, name + ' explicit reuse');
        if (name === 'DescriptorSetData') {
            assert.strictEqual(first.descriptorSetLayout, null);
            assert.strictEqual(first.descriptorSet, null);
            assert.strictEqual(second.descriptorSetLayout, layoutHandle);
            assert.strictEqual(second.descriptorSet, setHandle);
        }
    }
    return samples.map(({ name, explicit, defaults }) => ({ name, explicit, defaults }));
}

assert.deepEqual(exercise(current), exercise(legacy));
