const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const [jsDir, stage, engine] = process.argv.slice(2);
const types = require(path.join(jsDir, 'types.js'));
const { UpdateFrequency, AccessType, LightInfo, RenderCommonObjectPool } = types;
const { getUpdateFrequencyName } = require(path.join(jsDir, 'types-names.js'));
const { constEnumValues } = require(path.join(jsDir, 'enum-consumer.js'));

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
for (const name of ['UpdateFrequency', 'ResourceResidency', 'ResourceDimension', 'AccessType']) {
    compareEnum(name, 'enum');
}
compareDeclaration(cpp, 'struct LightInfo {', '};');
compareDeclaration(ts, 'export class LightInfo {', '}');
compareDeclaration(ts, '    createLightInfo (', '    }');

for (const [value, name] of ['PER_INSTANCE', 'PER_BATCH', 'PER_PHASE', 'PER_PASS', 'COUNT'].entries()) {
    assert.equal(UpdateFrequency[name], value);
    assert.equal(getUpdateFrequencyName(value), name);
}
assert.equal(getUpdateFrequencyName(99), '');
const runtimeMembers = {
    ResourceResidency: ['MANAGED', 'MEMORYLESS', 'PERSISTENT', 'EXTERNAL', 'BACKBUFFER', 'CUSTOM'],
    ResourceDimension: ['BUFFER', 'TEXTURE1D', 'TEXTURE2D', 'TEXTURE3D'],
};
for (const [name, members] of Object.entries(runtimeMembers)) {
    for (const [value, member] of members.entries()) {
        assert.equal(types[name][member], value);
        assert.equal(types[name][value], member);
    }
}
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
