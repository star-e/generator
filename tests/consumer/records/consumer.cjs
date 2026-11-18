const assert = require('assert').strict;
const fs = require('fs');
const path = require('path');
const [js, stage, engine] = process.argv.slice(2);
const { AccessType, LightInfo, RenderCommonObjectPool } = require(js);

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
compareDeclaration(cpp, 'enum class AccessType : uint8_t {', '};');
compareDeclaration(cpp, 'struct LightInfo {', '};');
compareDeclaration(ts, 'export enum AccessType {', '}');
compareDeclaration(ts, 'export class LightInfo {', '}');
compareDeclaration(ts, '    createLightInfo (', '    }');

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
