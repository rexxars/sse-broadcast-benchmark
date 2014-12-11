'use strict';

var argparse = require('argparse'),
    pkg      = require('../package.json'),
    parser   = new argparse.ArgumentParser({
        version: pkg.version,
        description: 'Small, synthetic benchmark of sse-broadcast implementations',
        addHelp: true
    });

parser.addArgument(['--port'], {
    help: 'Port number of the sse-broadcast server',
    defaultValue: 1942,
    dest: 'port',
    required: false,
    type: 'int'
});

parser.addArgument(['--host'], {
    help: 'Hostname of the sse-broadcast server',
    defaultValue: 'localhost',
    dest: 'host',
    required: false
});

parser.addArgument(['--num-clients'], {
    help: 'Number of SSE clients to connect',
    defaultValue: 3000,
    dest: 'numClients',
    type: 'int',
    required: false
});

parser.addArgument(['--probe-interval'], {
    help: 'Milliseconds between each probe',
    defaultValue: 500,
    dest: 'probeInterval',
    type: 'int',
    required: false
});

parser.addArgument(['--duration'], {
    help: 'How long the benchmark should last, in seconds',
    defaultValue: 30,
    dest: 'benchDuration',
    required: false
});

parser.addArgument(['--reset'], {
    help: 'Reset the previous results before writing the collected results',
    dest: 'reset',
    action: 'storeTrue'
});

parser.addArgument(['--profile-name'], {
    help: 'Name to use for this run - used for the resulting filename',
    dest: 'profileName',
    required: true
});

module.exports = function() {
    return parser.parseArgs();
};