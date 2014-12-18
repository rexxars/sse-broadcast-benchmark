'use strict';

var argparse = require('argparse'),
    parser   = new argparse.ArgumentParser({
        description: 'Spawn an sse-broadcast implementation and send system stats to a given HTTP endpoint',
        addHelp: true
    });

parser.addArgument(['--implementation'], {
    help: 'sse-broadcast implementation (must match folder name)',
    dest: 'implementation'
});

parser.addArgument(['--cmd'], {
    help: 'Command to run and monitor (alternative to --implementation)',
    dest: 'cmd'
});

parser.addArgument(['--port'], {
    help: 'Port number of the sse-broadcast implementation',
    defaultValue: 1942,
    dest: 'port',
    required: false,
    type: 'int'
});

parser.addArgument(['--endpoint'], {
    help: 'HTTP endpoint to send the statistics to',
    dest: 'endpoint',
    required: true
});

parser.addArgument(['--probe-interval'], {
    help: 'Milliseconds between each probe',
    defaultValue: 5000,
    dest: 'probeInterval',
    type: 'int',
    required: false
});

parser.addArgument(['--pid'], {
    help: 'If a PID is specified, it will collect statistics for that instead of spawning a new process',
    dest: 'pid',
    required: false,
    type: 'int'
});

module.exports = function() {
    return parser.parseArgs();
};