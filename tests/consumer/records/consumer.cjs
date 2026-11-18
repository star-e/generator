const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const [jsDir, stage, engine] = process.argv.slice(2);
const types = require(path.join(jsDir, 'types.js'));
const { UpdateFrequency, QueueHint, SceneFlags, AccessType, LightInfo, RenderCommonObjectPool } = types;
const { getUpdateFrequencyName } = require(path.join(jsDir, 'types-names.js'));
const { constEnumValues, attachmentUsage, usesStorage, resolveFlags, depthStencil, resolvesColor, highBit }
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
compareDeclaration(ts, 'export class LightInfo {', '}');
compareDeclaration(ts, '    createLightInfo (', '    }');
for (const op of ['==', '!=']) {
    compareDeclaration(cpp, `inline bool operator${op}(const ResolvePair& lhs, const ResolvePair& rhs) noexcept {`, '}');
}
const sourceFile = cpp.replace(/\.h$/, '.cpp');
function constructors(root, name) {
    return fs.readFileSync(path.join(root, sourceFile), 'utf8').replace(/\r\n/g, '\n').split('\n\n')
        .filter((block) => block.startsWith(`${name}::${name}(`));
}
for (const name of ['ResolvePair', 'CopyPair', 'MovePair']) {
    compareDeclaration(cpp, `struct ${name} {`, '};');
    compareDeclaration(ts, `export class ${name} {`, '}');
    compareDeclaration(ts, `    create${name} (`, '    }');
    const definitions = constructors(stage, name);
    assert.equal(definitions.length, 4, name);
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
