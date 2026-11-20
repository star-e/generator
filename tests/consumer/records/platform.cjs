// Stand in for the virtual module supplied by the engine build.
const Module = require('module');
const flags = { COCOS_RUNTIME: false, HTML5: false };
const load = Module._load;
Module._load = function (name, ...args) {
    return name === 'internal:constants' ? flags : load.call(this, name, ...args);
};
module.exports = flags;
