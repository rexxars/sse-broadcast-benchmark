#!/bin/sh
// 2>/dev/null; exec "$(command -v nodejs || command -v node)" "$0" "$@"

'use strict';

var spawn   = require('child_process').spawn,
    request = require('request'),
    fs      = require('fs'),
    os      = require('os'),
    usage   = require('usage'),
    args    = require('./args')(),
    impFile = __dirname + '/../' + args.implementation + '/server',
    cmdArgs = ['--port', args.port],
    cmd     = impFile,
    pid;

if (!args.pid && !args.implementation && !args.cmd) {
    console.error('You must specify either --implementation, --cmd or --pid');
    process.exit(1);
} else if (args.implementation && !fs.existsSync(impFile)) {
    console.error('Implementation does not seem to exist (' + args.implementation + '/server)');
    process.exit(2);
} else if (args.cmd) {
    cmdArgs = args.cmd.split(' ');
    cmd = cmdArgs.shift();
}

if (!args.pid) {
    var sse = spawn(cmd, cmdArgs);
    sse.stdout.pipe(process.stdout);
    sse.stderr.pipe(process.stderr);
}

pid = args.pid || sse.pid;

setInterval(gatherStats, args.probeInterval);

function gatherStats() {
    usage.lookup(sse.pid, { keepHistory: true }, function(err, result) {
        if (err) {
            return console.error(err);
        }

        result.timestamp = Date.now();
        result.loadAvg   = os.loadavg();
        result.hostname  = os.hostname();
        result.ssePort   = args.port;
        result.freeMem   = os.freemem();

        reportStats(result);
    });
}

function reportStats(stats) {
    request({
        uri: args.endpoint,
        method: 'POST',
        json: stats
    }, onStatsReported);
}

function onStatsReported(err, res, body) {
    if (err || res.statusCode >= 400) {
        return console.error('Failed reporting statistics: ', err || 'HTTP ' + res.statusCode);
    }

    console.log(body);
}