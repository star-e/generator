const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const [jsDir, stage, engine, nativeFile, webFile] = process.argv.slice(2);
const types = require(path.join(jsDir, 'types.js'));
const { UpdateFrequency, QueueHint, SceneFlags, AccessType, LightInfo, RenderCommonObjectPool } = types;
const { getUpdateFrequencyName } = require(path.join(jsDir, 'types-names.js'));
const { constEnumValues, attachmentUsage, usesStorage, resolveFlags, depthStencil, resolvesColor, highBit, layoutEnums, descriptorBlockArgs }
    = require(path.join(jsDir, 'enum-consumer.js'));

function compareDeclaration(file, firstLine, lastLine) {
    function read(root) {
        const lines = fs.readFileSync(path.join(root, file), 'utf8').split(/\r?\n/);
        const start = lines.indexOf(firstLine);
        assert.ok(start >= 0, firstLine);
        const end = lines.indexOf(lastLine, start + 1);
        assert.ok(end > start, lastLine);
        return lines.slice(start, end + 1);
    }
    assert.deepEqual(read(stage), read(engine), firstLine);
}

const cpp = 'native/cocos/renderer/pipeline/custom/RenderCommonTypes.h';
const ts = 'cocos/rendering/custom/types.ts';
const layoutCpp = 'native/cocos/renderer/pipeline/custom/LayoutGraphTypes.h';
const layoutTs = 'cocos/rendering/custom/layout-graph.ts';
const layout = require(path.join(jsDir, 'layout-graph.js'));
for (const [name, values] of Object.entries(layoutEnums)) {
    compareDeclaration(layoutCpp, `enum class ${name} : uint8_t {`, '};');
    compareDeclaration(layoutTs, `export const enum ${name} {`, '}');
    assert.deepEqual(values, values.map((_, index) => index), name);
    assert.equal(layout[name], undefined, name);
}
compareDeclaration(layoutCpp, 'struct Descriptor {', '};');
compareDeclaration(layoutCpp, 'struct DescriptorBlockIndex {', '};');
compareDeclaration(layoutCpp, 'inline bool operator<(const DescriptorBlockIndex& lhs, const DescriptorBlockIndex& rhs) noexcept {', '}');
compareDeclaration(layoutTs, 'export class Descriptor {', '}');
const { getDescriptorTypeOrderName } = require(path.join(jsDir, 'layout-graph-names.js'));
assert.deepEqual(layoutEnums.DescriptorTypeOrder.map(getDescriptorTypeOrderName), [
    'UNIFORM_BUFFER', 'DYNAMIC_UNIFORM_BUFFER', 'SAMPLER_TEXTURE', 'SAMPLER', 'TEXTURE',
    'STORAGE_BUFFER', 'DYNAMIC_STORAGE_BUFFER', 'STORAGE_IMAGE', 'INPUT_ATTACHMENT',
]);
assert.equal(getDescriptorTypeOrderName(255), '');
const { Type } = require(path.resolve(jsDir, '../../gfx'));
const { DescriptorBlockIndex: EngineIndex } = require(path.join(jsDir, 'engine-index.js'));
const blocks = new Map();
for (let count = 0; count <= descriptorBlockArgs.length; ++count) {
    const args = descriptorBlockArgs.slice(0, count);
    const engineKey = JSON.stringify(new EngineIndex(...args));
    blocks.set(engineKey, count);
    assert.equal(blocks.get(JSON.stringify(new layout.DescriptorBlockIndex(...args))), count);
}
assert.equal(blocks.size, 5);
const descriptor = new layout.Descriptor();
assert.equal(descriptor.type, Type.UNKNOWN);
assert.equal(descriptor.count, 1);
const typedDescriptor = new layout.Descriptor(Type.FLOAT4);
assert.equal(typedDescriptor.type, Type.FLOAT4);
typedDescriptor.count = 9;
typedDescriptor.reset(Type.SAMPLER);
assert.equal(typedDescriptor.type, Type.SAMPLER);
assert.equal(typedDescriptor.count, 1);
const engineTypes = fs.readFileSync(path.join(engine, ts), 'utf8').split(/\r?\n/);
for (const line of fs.readFileSync(path.join(stage, ts), 'utf8').split(/\r?\n/)) {
    if (line.startsWith('import ')) assert.ok(engineTypes.includes(line), line);
}
function compareEnum(name, tsKind) {
    compareDeclaration(cpp, `enum class ${name} : uint8_t {`, '};');
    compareDeclaration(ts, `export ${tsKind} ${name} {`, '}');
}
for (const [name, values] of Object.entries(constEnumValues)) {
    compareEnum(name, 'const enum');
    assert.deepEqual(values, values.map((_, index) => index), name);
    assert.equal(types[name], undefined, name);
}
for (const name of ['UpdateFrequency', 'ResourceResidency', 'QueueHint', 'ResourceDimension', 'AccessType']) {
    compareEnum(name, 'enum');
}
for (const name of ['ResourceFlags', 'SceneFlags', 'ResolveFlags']) {
    const anyFlags = `constexpr bool any(${name} e) noexcept {`;
    compareDeclaration(cpp, `enum class ${name} : uint32_t { // NOLINT(performance-enum-size)`, anyFlags);
    compareDeclaration(cpp, anyFlags, '}');
}
compareDeclaration(ts, 'export enum ResourceFlags {', '}');
compareDeclaration(ts, 'export enum SceneFlags {', '}');
for (const [value, member, alias] of [
    [1, 'OPAQUE', 'OPAQUE_OBJECT'],
    [2, 'MASK', 'CUTOUT_OBJECT'],
    [4, 'BLEND', 'TRANSPARENT_OBJECT'],
]) {
    assert.equal(SceneFlags[member], value);
    assert.equal(SceneFlags[alias], value);
    assert.equal(SceneFlags[value], alias);
}
assert.equal(SceneFlags.ALL, 4294967295);
assert.equal(SceneFlags.ALL & SceneFlags.UI, 0x10);
assert.equal(SceneFlags.ALL & SceneFlags.PROFILER, 0x400);
compareDeclaration(ts, 'export const enum ResolveFlags {', '}');
assert.equal(types.ResolveFlags, undefined);
assert.deepEqual(resolveFlags, [0, 1, 2, 4]);
assert.equal(depthStencil, 6);
assert.equal(resolvesColor, false);
assert.equal(highBit, 0x80000000);
assert.equal(attachmentUsage, 0x18);
assert.equal(usesStorage(attachmentUsage), false);
assert.equal(usesStorage(attachmentUsage | types.ResourceFlags.STORAGE), true);
assert.equal(types.ResourceFlags[0x200], 'TRANSFER_DST');
compareDeclaration(cpp, 'struct LightInfo {', '};');
compareDeclaration(cpp, 'struct RasterPassTag {};', 'struct RaytraceTag {};');
compareDeclaration(cpp, 'struct BufferTag {};', 'struct TextureTag {};');
compareDeclaration(ts, 'export class LightInfo {', '}');
compareDeclaration(ts, 'export class RenderCommonObjectPool {', '}');
compareDeclaration(cpp, 'struct PipelineStatistics {', '};');
compareDeclaration(ts, 'export class PipelineStatistics {', '}');
function checkStatistics({ PipelineStatistics, RenderCommonObjectPool }, renderPasses) {
    const stats = new PipelineStatistics();
    const pool = new RenderCommonObjectPool();
    const pooled = pool.createPipelineStatistics();
    assert.ok(pooled instanceof PipelineStatistics);
    assert.notEqual(pool.createPipelineStatistics(), pooled);
    assert.deepEqual(pooled, stats);
    const counters = Object.keys(stats);
    assert.equal(counters.length, 11);
    assert.ok(counters.includes('numRenderPasses'));
    for (const [index, name] of counters.entries()) {
        assert.equal(stats[name], name === 'numRenderPasses' ? renderPasses : 0, name);
        stats[name] = 100 + index;
        pooled[name] = 200 + index;
    }
    stats.reset();
    pool.reset();
    const reused = pool.createPipelineStatistics();
    assert.equal(reused, pooled);
    for (const name of counters) {
        assert.equal(stats[name], name === 'numRenderPasses' ? renderPasses : 0, name);
        assert.equal(reused[name], stats[name], name);
    }
}
checkStatistics(types, 0);
checkStatistics(require(path.join(jsDir, 'types-variant.js')), 7);
for (const op of ['==', '!=']) {
    compareDeclaration(cpp, `inline bool operator${op}(const ResolvePair& lhs, const ResolvePair& rhs) noexcept {`, '}');
}
const sourceFile = cpp.replace(/\.h$/, '.cpp');
compareDeclaration(cpp, 'inline hash_t hash<cc::render::ResolvePair>::operator()(const cc::render::ResolvePair& val) const noexcept {', '}');
function constructors(root, name) {
    return fs.readFileSync(path.join(root, sourceFile), 'utf8').replace(/\r\n/g, '\n').split('\n\n')
        .filter((block) => block.startsWith(`${name}::${name}(`));
}
for (const name of ['ResolvePair', 'CopyPair', 'UploadPair', 'MovePair']) {
    compareDeclaration(cpp, `struct ${name} {`, '};');
    compareDeclaration(ts, `export class ${name} {`, '}');
    const definitions = constructors(stage, name);
    assert.equal(definitions.length, name === 'UploadPair' ? 3 : 4, name);
    assert.deepEqual(definitions, constructors(engine, name), name);
}

for (const [value, name] of ['PER_INSTANCE', 'PER_BATCH', 'PER_PHASE', 'PER_PASS', 'COUNT'].entries()) {
    assert.equal(UpdateFrequency[name], value);
    assert.equal(getUpdateFrequencyName(value), name);
}
assert.equal(getUpdateFrequencyName(99), '');
const runtimeMembers = {
    ResourceResidency: ['MANAGED', 'MEMORYLESS', 'PERSISTENT', 'EXTERNAL', 'BACKBUFFER', 'CUSTOM'],
    QueueHint: ['NONE', 'RENDER_OPAQUE', 'RENDER_CUTOUT', 'RENDER_TRANSPARENT'],
    ResourceDimension: ['BUFFER', 'TEXTURE1D', 'TEXTURE2D', 'TEXTURE3D'],
};
for (const [name, members] of Object.entries(runtimeMembers)) {
    for (const [value, member] of members.entries()) {
        assert.equal(types[name][member], value);
        assert.equal(types[name][value], member);
    }
}
assert.equal(QueueHint.RENDER_OPAQUE, QueueHint.OPAQUE);
assert.equal(QueueHint.RENDER_CUTOUT, QueueHint.MASK);
assert.equal(QueueHint.RENDER_TRANSPARENT, QueueHint.BLEND);
assert.deepEqual([AccessType.READ, AccessType.READ_WRITE, AccessType.WRITE], [0, 1, 2]);
const fields = (v) => [v.light, v.level, v.culledByLight, v.probe];
assert.deepEqual(fields(new LightInfo()), [null, 0, false, null]);
const light = {}, probe = {};
assert.deepEqual(fields(new LightInfo(light, 3)), [light, 3, false, null]);

const pool = new RenderCommonObjectPool();
const a = pool.createLightInfo(light, 17, true, probe);
assert.equal(a.light, light);
assert.equal(a.probe, probe);
assert.deepEqual(fields(a), [light, 17, true, probe]);
const b = pool.createLightInfo();
assert.notEqual(a, b);
pool.reset();
assert.equal(pool.createLightInfo(), a);
assert.deepEqual(fields(a), [null, 0, false, null]);
assert.equal(pool.createLightInfo(light, 9, false, probe), b);
assert.deepEqual(fields(b), [light, 9, false, probe]);

const { ResolveMode } = require(path.join(jsDir, '../../gfx'));
const pairFields = (p) => [p.source, p.target, p.resolveFlags, p.mode, p.mode1];
const defaults = ['', '', 0, ResolveMode.SAMPLE_ZERO, ResolveMode.SAMPLE_ZERO];
const explicit = ['color-msaa', 'color', depthStencil, ResolveMode.MIN, ResolveMode.MAX];
assert.deepEqual(pairFields(new types.ResolvePair()), defaults);
assert.deepEqual(pairFields(new types.ResolvePair(...explicit)), explicit);
const pair = pool.createResolvePair(...explicit);
assert.deepEqual(pairFields(pair), explicit);
pool.reset();
assert.equal(pool.createResolvePair(), pair);
assert.deepEqual(pairFields(pair), defaults);

function checkSubresources(name, offsets) {
    const names = ['source', 'target', 'mipLevels', 'numSlices', ...offsets];
    const values = (p) => names.map((field) => p[field]);
    const initial = ['', '', 4294967295, 4294967295, ...offsets.map(() => 0)];
    const explicit = ['source', 'target', 1, 2, ...offsets.map((_, i) => i + 3)];
    assert.deepEqual(values(new types[name]()), initial, name);
    assert.deepEqual(values(new types[name](...explicit)), explicit, name);
    const pair = pool[`create${name}`](...explicit);
    assert.deepEqual(values(pair), explicit, name);
    pool.reset();
    assert.equal(pool[`create${name}`](), pair, name);
    assert.deepEqual(values(pair), initial, name);
}
const targetOffsets = ['targetMostDetailedMip', 'targetFirstSlice', 'targetPlaneSlice'];
checkSubresources('CopyPair', ['sourceMostDetailedMip', 'sourceFirstSlice', 'sourcePlaneSlice', ...targetOffsets]);
checkSubresources('MovePair', targetOffsets);

const uploadFields = (p) => [p.target, p.mipLevels, p.numSlices, p.targetMostDetailedMip, p.targetFirstSlice, p.targetPlaneSlice];
const uploadDefaults = ['', 4294967295, 4294967295, 0, 0, 0];
const bytes = new Uint8Array([0, 127, 255]);
const upload = new types.UploadPair(bytes, 'texture', 1, 2, 3, 4, 5);
assert.deepEqual(uploadFields(upload), ['texture', 1, 2, 3, 4, 5]);
upload.reset('other', 6, 7, 8, 9, 10);
assert.deepEqual(uploadFields(upload), ['other', 6, 7, 8, 9, 10]);
assert.equal(upload.source, bytes);
assert.deepEqual(Array.from(bytes), [0, 127, 255]);

const pooledUpload = pool.createUploadPair('texture', 1, 2, 3, 4, 5);
assert.deepEqual(uploadFields(pooledUpload), ['texture', 1, 2, 3, 4, 5]);
const pooledBytes = pooledUpload.source;
assert.ok(pooledBytes instanceof Uint8Array);
assert.equal(pooledBytes.length, 0);
pool.reset();
assert.equal(pool.createUploadPair(), pooledUpload);
assert.equal(pooledUpload.source, pooledBytes);
assert.deepEqual(uploadFields(pooledUpload), uploadDefaults);

const { BinaryInputArchive, BinaryOutputArchive } = require(path.join(jsDir, 'binary-archive.js'));
const statistics = Object.assign(new types.PipelineStatistics(), {
    numRenderPasses: 21, numManagedTextures: 22, totalManagedTextures: 23, numUploadBuffers: 24,
    numUploadBufferViews: 25, numFreeUploadBuffers: 26, numFreeUploadBufferViews: 27, numDescriptorSets: 28,
    numFreeDescriptorSets: 29, numInstancingBuffers: 30, numInstancingUniformBlocks: 0xFFFFFFFF,
});
const fixtures = [
    ['LightInfo', new LightInfo(light, 77, true, probe)],
    ['ResolvePair', new types.ResolvePair('color-msaa', 'color', depthStencil, ResolveMode.MIN, ResolveMode.MAX)],
    ['CopyPair', new types.CopyPair('copy-src', 'copy-dst', 1, 2, 3, 4, 5, 6, 7, 8)],
    ['MovePair', new types.MovePair('move-src', 'move-dst', 11, 12, 13, 14, 15)],
    ['PipelineStatistics', statistics],
];
const bytesFromCpp = fs.readFileSync(nativeFile);
const input = new BinaryInputArchive(bytesFromCpp.buffer.slice(bytesFromCpp.byteOffset, bytesFromCpp.byteOffset + bytesFromCpp.byteLength));
const output = new BinaryOutputArchive();
for (const [name, expected] of fixtures) {
    compareDeclaration(ts, 'export function save' + name + ' (a: OutputArchive, v: ' + name + '): void {', '}');
    compareDeclaration(ts, 'export function load' + name + ' (a: InputArchive, v: ' + name + '): void {', '}');
    const actual = new types[name]();
    if (name === 'LightInfo') Object.assign(actual, { light, probe });
    types['load' + name](input, actual);
    assert.deepEqual(actual, expected, name);
    if (name === 'LightInfo') {
        assert.equal(actual.light, light);
        assert.equal(actual.probe, probe);
    }
    types['save' + name](output, expected);
}
assert.equal(input.offset, bytesFromCpp.byteLength);
assert.equal(types.saveUploadPair, undefined);
assert.equal(types.loadUploadPair, undefined);
fs.writeFileSync(webFile, new Uint8Array(output.data));
