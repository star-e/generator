const path = require('path');

module.exports = function loadLayoutArchive(directory) {
    return {
        ...require(path.join(directory, 'layout-graph.js')),
        ...require(path.join(directory, 'binary-archive.js')),
    };
};
