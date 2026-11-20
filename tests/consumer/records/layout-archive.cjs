const assert = require('assert').strict;
require('./platform.cjs');
const fs = require('fs');
const path = require('path');
const [jsDir, nativeFile, webFile, legacyFile, invalidFile, legacyFixture, blocksNativeFile, blocksWebFile, blocksLegacyFile] = process.argv.slice(2);
const current = require(path.join(jsDir, 'layout-graph.js'));
const legacy = require(path.join(jsDir, 'legacy-layout.js'));
const { BinaryInputArchive, BinaryOutputArchive } = require(path.join(jsDir, 'binary-archive.js'));
const { UpdateFrequency } = require(path.join(jsDir, 'types.js'));
const { descriptorBlockDataArgs } = require(path.join(jsDir, 'enum-consumer.js'));
const { Format, Type, Uniform, UniformBlock } = require(path.resolve(jsDir, '../../gfx'));
const stage = 0, phase = 1;

function fixture(api) {
    const g = new api.LayoutGraph();
    const addStage = (name, payload, parent) => g.addVertex(stage, payload, name, new api.DescriptorDB(), parent);
    const main = addStage('main', 1);
    const other = addStage('other', 0);
    const db = new api.DescriptorDB();
    const shadow = new api.RenderPhase();
    shadow.shaders.add('alpha').add('zeta');
    g.addVertex(phase, shadow, 'shadow', db, main);
    g.addVertex(phase, new api.RenderPhase(), 'shadow', new api.DescriptorDB(), other);
    addStage('sub', 0, main);
    const block = new api.DescriptorBlock();
    block.count = 3;
    block.capacity = 5;
    const desc = new api.Descriptor(Type.SAMPLER2D);
    desc.count = 3;
    block.descriptors.set('atlas', desc);
    block.uniformBlocks.set('scene', new UniformBlock(1, 7, 'Scene', [new Uniform('tint', Type.FLOAT4, 2)], 4));
    db.blocks.set(JSON.stringify(new api.DescriptorBlockIndex(UpdateFrequency.PER_PASS)), block);
    const group = new api.DescriptorGroupBlockIndex(UpdateFrequency.PER_PHASE);
    group.format = Format.RGBA8;
    const groupBlock = new api.DescriptorBlock();
    groupBlock.capacity = 9;
    db.groupBlocks.set(JSON.stringify(group), groupBlock);
    return g;
}

function encode(api) {
    const out = new BinaryOutputArchive();
    api.saveLayoutGraph(out, new api.LayoutGraph());
    api.saveLayoutGraph(out, fixture(api));
    return Buffer.from(out.data);
}

function input(bytes) {
    return new BinaryInputArchive(bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength));
}

function verify(api, bytes) {
    const archive = input(bytes);
    const g = new api.LayoutGraph();
    api.loadLayoutGraph(archive, g);
    assert.equal(g.nv(), 0);
    api.loadLayoutGraph(archive, g);
    assert.equal(archive.offset, bytes.length);
    const expected = fixture(api);
    assert.deepEqual([...g.v()], [...expected.v()]);
    for (const v of expected.v()) {
        assert.equal(g.w(v), expected.w(v));
        assert.equal(g.getName(v), expected.getName(v));
        assert.equal(g.getParent(v), expected.getParent(v));
        assert.deepEqual(g.getDescriptors(v), expected.getDescriptors(v));
        assert.deepEqual(g.value(g.w(v), v), expected.value(expected.w(v), v));
    }
    assert.equal(g.locate('/other/shadow'), 3);
}

const previousBytes = encode(legacy);
const webBytes = encode(current);
const nativeBytes = fs.readFileSync(nativeFile);
assert.deepEqual(webBytes, previousBytes);
assert.deepEqual(nativeBytes, previousBytes);
verify(current, previousBytes);
verify(current, fs.readFileSync(legacyFixture));
verify(current, nativeBytes);
verify(legacy, webBytes);
verify(legacy, nativeBytes);
fs.writeFileSync(webFile, webBytes);
fs.writeFileSync(legacyFile, previousBytes);

// The first empty graph occupies four numbers. Corrupt the second graph's edge
// count so loading fails after its last vertex, then retry on the same object.
const valid = previousBytes.subarray(4 * 8);
const invalid = Buffer.from(valid);
invalid.writeDoubleLE(invalid.readDoubleLE(8) + 1, 8);
fs.writeFileSync(invalidFile, invalid);
const reused = new current.LayoutGraph();
reused.addVertex(stage, 0, 'previous', new current.DescriptorDB());
assert.throws(() => current.loadLayoutGraph(input(invalid), reused), /archive counts/);
assert.equal(reused.nv(), 1);
assert.equal(reused.locate('/previous'), 0);
current.loadLayoutGraph(input(valid), reused);
assert.equal(reused.nv(), 5);
assert.equal(reused.locate('/main/shadow'), 2);

function flattened(api, size) {
    const value = new api.DescriptorBlockFlattened();
    for (let i = 2 - size; i < 2; ++i) {
        value.descriptorNames.push(i === 0 ? 'z-depth' : 'albedo');
        value.uniformBlockNames.push(i === 0 ? 'u-view' : 'u-draw');
        const descriptor = new api.Descriptor(i === 0 ? Type.SAMPLER2D : Type.FLOAT4);
        descriptor.count = i + 2;
        value.descriptors.push(descriptor);
        value.uniformBlocks.push(new UniformBlock(i + 1, i + 7, i === 0 ? 'View' : 'Draw',
            i === 0 ? [new Uniform('tint', Type.FLOAT4, 2)] : [], i + 4));
    }
    value.capacity = size * 4;
    value.count = size * 2;
    return value;
}

function uniformBlock(api, size) {
    const block = new api.UniformBlockData();
    block.bufferSize = size * 64;
    if (size === 2) block.uniforms.push(new api.UniformData());
    if (size) {
        const member = new api.UniformData(0x80000009, Type.FLOAT4, 32);
        member.size = 16;
        block.uniforms.push(member);
    }
    return block;
}

const sizes = [0, 2, 1, 0];
const ids = [0, 23, 0xFFFFFFFF];
function descriptorBlock(api, size) {
    if (size === 0) return new api.DescriptorBlockData();
    const args = size === 1 ? descriptorBlockDataArgs.slice(0, 3) : descriptorBlockDataArgs;
    const block = new api.DescriptorBlockData(...args);
    block.offset = size + 5;
    if (size === 2) block.descriptors.push(new api.DescriptorData(0xFFFFFFFF, Type.FLOAT4, 3));
    block.descriptors.push(new api.DescriptorData(0, Type.SAMPLER2D, 2));
    return block;
}

const setSizes = [0, 1, 2, 0];
function setLayout(api, size) {
    if (!size) return new api.DescriptorSetLayoutData();
    const blocks = [], uniforms = new Map(), bindings = new Map();
    for (let i = 1; i <= size; ++i) blocks.push(descriptorBlock(api, i));
    for (const key of [42, 17].slice(0, size)) {
        uniforms.set(key, new UniformBlock(size, key, 'Scene', [new Uniform('tint', Type.FLOAT4, key)], 2));
        bindings.set(key, key + size);
    }
    const layout = new api.DescriptorSetLayoutData(size, size * 8, blocks, uniforms, bindings);
    assert.strictEqual(layout.descriptorBlocks, blocks);
    assert.strictEqual(layout.uniformBlocks, uniforms);
    assert.strictEqual(layout.bindingMap, bindings);
    layout.uniformBlockCapacity = size * 3;
    layout.samplerTextureCapacity = size * 5;
    return layout;
}

function encodeBlocks(api, layoutSizes = setSizes) {
    const output = new BinaryOutputArchive();
    for (const size of sizes) {
        api.saveDescriptorBlockFlattened(output, flattened(api, size));
        api.saveUniformBlockData(output, uniformBlock(api, size));
        api.saveDescriptorBlockData(output, descriptorBlock(api, size));
    }
    for (const id of ids) {
        output.n(id);
        api.saveDescriptorData(output, new api.DescriptorData(id, Type.FLOAT4, 3));
    }
    for (const size of layoutSizes) api.saveDescriptorSetLayoutData(output, setLayout(api, size));
    return Buffer.from(output.data);
}

function verifyBlocks(api, bytes) {
    const archive = input(bytes);
    const value = flattened(api, 2);
    const fields = ['descriptorNames', 'uniformBlockNames', 'descriptors', 'uniformBlocks'];
    const refs = fields.map(field => value[field]);
    const block = uniformBlock(api, 2);
    const uniforms = block.uniforms;
    const descriptorData = descriptorBlock(api, 2);
    const descriptors = descriptorData.descriptors;
    for (const size of sizes) {
        api.loadDescriptorBlockFlattened(archive, value);
        fields.forEach((field, i) => assert.strictEqual(value[field], refs[i], field));
        assert.deepEqual(value, flattened(api, size));
        api.loadUniformBlockData(archive, block);
        assert.strictEqual(block.uniforms, uniforms);
        assert.deepEqual(block, uniformBlock(api, size));
        api.loadDescriptorBlockData(archive, descriptorData);
        assert.strictEqual(descriptorData.descriptors, descriptors);
        assert.deepEqual(descriptorData, descriptorBlock(api, size));
    }
    const descriptor = new api.DescriptorData();
    for (const id of ids) {
        assert.equal(archive.n(), id);
        api.loadDescriptorData(archive, descriptor);
        assert.deepEqual(descriptor, new api.DescriptorData(id, Type.FLOAT4, 3));
    }
    const layout = new api.DescriptorSetLayoutData();
    const { descriptorBlocks, uniformBlocks, bindingMap } = layout;
    const expectedUniforms = new Map(), expectedBindings = new Map();
    for (const size of setSizes) {
        api.loadDescriptorSetLayoutData(archive, layout);
        const expected = setLayout(api, size);
        for (const key of ['slot', 'capacity', 'uniformBlockCapacity', 'samplerTextureCapacity']) assert.equal(layout[key], expected[key]);
        assert.strictEqual(layout.descriptorBlocks, descriptorBlocks);
        assert.strictEqual(layout.uniformBlocks, uniformBlocks);
        assert.strictEqual(layout.bindingMap, bindingMap);
        assert.deepEqual(descriptorBlocks, expected.descriptorBlocks);
        // The engine's TS loader overwrites matching keys and preserves keys absent from the archive.
        for (const [key, uniform] of expected.uniformBlocks) expectedUniforms.set(key, uniform);
        for (const [key, binding] of expected.bindingMap) expectedBindings.set(key, binding);
        assert.deepEqual(uniformBlocks, expectedUniforms);
        assert.deepEqual(bindingMap, expectedBindings);
    }
    assert.equal(archive.offset, bytes.length);
}

const blocksLegacy = encodeBlocks(legacy);
const blocksWeb = encodeBlocks(current);
const blocksNative = fs.readFileSync(blocksNativeFile);
assert.deepEqual(blocksWeb, blocksLegacy);
// The empty and single-key layouts have deterministic bytes. Multi-key maps can be traversed in different orders.
const prefix = encodeBlocks(legacy, [0, 1]);
assert.deepEqual(blocksNative.subarray(0, prefix.length), prefix);
assert.deepEqual(blocksWeb.subarray(0, prefix.length), prefix);
verifyBlocks(current, blocksNative);
verifyBlocks(current, blocksLegacy);
verifyBlocks(legacy, blocksWeb);
verifyBlocks(legacy, blocksNative);
fs.writeFileSync(blocksWebFile, blocksWeb);
fs.writeFileSync(blocksLegacyFile, blocksLegacy);

const layout = setLayout(current, 2);
const { descriptorBlocks, uniformBlocks, bindingMap } = layout;
layout.reset(9, 12);
assert.deepEqual(layout, new current.DescriptorSetLayoutData(9, 12));
assert.strictEqual(layout.descriptorBlocks, descriptorBlocks);
assert.strictEqual(layout.uniformBlocks, uniformBlocks);
assert.strictEqual(layout.bindingMap, bindingMap);
const first = new current.DescriptorSetLayoutData(), second = new current.DescriptorSetLayoutData();
for (const key of ['descriptorBlocks', 'uniformBlocks', 'bindingMap']) assert.notStrictEqual(first[key], second[key]);

const resetBlock = uniformBlock(current, 2);
const uniforms = resetBlock.uniforms;
const member = uniforms[1];
member.reset(7, Type.SAMPLER2D, 12);
assert.deepEqual(member, new current.UniformData(7, Type.SAMPLER2D, 12));
resetBlock.reset();
assert.strictEqual(resetBlock.uniforms, uniforms);
assert.deepEqual(resetBlock, new current.UniformBlockData());

const data = new current.DescriptorBlockData();
const descriptors = data.descriptors;
data.offset = 9;
descriptors.push(new current.DescriptorData(23, Type.SAMPLER2D, 2));
data.reset(...descriptorBlockDataArgs);
assert.deepEqual(data, new current.DescriptorBlockData(...descriptorBlockDataArgs));
assert.strictEqual(data.descriptors, descriptors);

assert.equal(current.NameLocalID, undefined);
assert.equal(current.saveNameLocalID, undefined);
assert.equal(current.loadNameLocalID, undefined);
const descriptor = new current.DescriptorData();
assert.deepEqual([descriptor.descriptorID, descriptor.type, descriptor.count], [0, Type.UNKNOWN, 1]);
descriptor.reset(0xFFFFFFFF, Type.SAMPLER2D, 7);
assert.deepEqual([descriptor.descriptorID, descriptor.type, descriptor.count], [0xFFFFFFFF, Type.SAMPLER2D, 7]);
const { LocalIDProbe } = require(path.join(jsDir, 'shift-probe.js'));
const local = new LocalIDProbe();
assert.equal(local.id, 0xFFFFFFFF);
local.values.set(23, 0);
assert.equal(local.values.get(23), 0);
