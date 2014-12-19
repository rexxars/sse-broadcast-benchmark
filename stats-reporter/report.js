#!/bin/sh
// 2>/dev/null; exec "$(command -v nodejs || command -v node)" "$0" "$@"

'use strict';

var EventSource = require('eventsource'),
    spawn   = require('child_process').spawn,
    request = require('request'),
    fs      = require('fs'),
    async   = require('async'),
    os      = require('os'),
    usage   = require('usage'),
    args    = require('./args')(),
    host    = 'http://localhost:' + args.port,
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
    async.parallel({
        systemInfo: getSystemInfo,
        networkTiming: getNetworkTiming,
        connectionCount: getConnectionCount
    }, function(err, results) {
        if (err) {
            return;
        }

        reportStats(results);
    });
}

function getSystemInfo(callback) {
    usage.lookup(pid, { keepHistory: true }, function(err, result) {
        if (err) {
            console.error('Failed to get system info', err);
            return callback(err);
        }

        result.loadAvg   = os.loadavg();
        result.hostname  = os.hostname();
        result.ssePort   = args.port;
        result.freeMem   = os.freemem();

        callback(null, result);
    });
}

function getNetworkTiming(callback) {
    var responseTime;
    var es = new EventSource(host + '/sse');
    var start = Date.now();
    var timeoutId;
    var scheduleBroadcast = function() {
        timeoutId = setTimeout(function() {
            request.post({
                headers: {'content-type': 'text/plain'},
                url: host + '/broadcast',
                body: ''+Date.now()
            }, function(error, response, body) {});
        }, 1000);
    }

    es.onopen = function() {
        responseTime = Date.now() - start;
        scheduleBroadcast();
    };
    es.onerror = function(err) {
        console.error('Failed to request /sse when probing for response time', err);
        if (timeoutId != -1) {
          clearTimeout(timeoutId);
          timeoutId = -1;
        }
        es.close();
        callback(err);
    };
    es.onmessage = function(msg) {
        es.close();
        callback(null, {
            responseTime: responseTime,
            deliveryTime: Date.now() - parseInt(msg.data, 10)
        });
    };
}

function getConnectionCount(callback) {
    request(host + '/connections', function(err, res, body) {
        callback(err, body | 0);
    });
}

function reportStats(stats) {
    stats.timestamp = Date.now();
    request({
        uri: args.endpoint,
        method: 'POST',
        json: stats
    }, onStatsReported);
}

function onStatsReported(err, res) {
    if (err || res.statusCode >= 400) {
        return console.error('Failed reporting statistics: ', err || 'HTTP ' + res.statusCode);
    }
}
