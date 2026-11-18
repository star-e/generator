const assert = require('assert').strict;
const fs = require('fs');
const { BinaryInputArchive, BinaryOutputArchive } = require(process.argv[2]);

const data = fs.readFileSync(process.argv[3]);
const reader = new BinaryInputArchive(data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength));
assert.equal(reader.b(), true);
assert.equal(reader.b(), false);
assert.equal(reader.n(), -17.25);
assert.equal(reader.s(), 'Cocos archive');
assert.equal(reader.offset, data.byteLength);

const writer = new BinaryOutputArchive();
writer.b(true);
writer.b(false);
writer.n(-17.25);
writer.s('Cocos archive');
fs.writeFileSync(process.argv[4], new Uint8Array(writer.data));
