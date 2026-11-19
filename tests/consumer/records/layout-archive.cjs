const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const [jsDir, nativeFile, webFile, legacyFile, invalidFile, legacyFixture] = process.argv.slice(2);
const current = require(path.join(jsDir, 'layout-graph.js'));
const legacy = require(path.join(jsDir, 'legacy-layout.js'));
const { BinaryInputArchive, BinaryOutputArchive } = require(path.join(jsDir, 'binary-archive.js'));
const { UpdateFrequency } = require(path.join(jsDir, 'types.js'));
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
