#!/bin/sh
// 2>/dev/null; exec "$(command -v nodejs || command -v node)" "$0" "$@"

'use strict';

var args = require('./args')();
var http = require('http');
var fs = require('fs');
var os = require('os');
var humanize = require('humanize');
var Table = require('cli-table');
var EventSource = require('eventsource');
var numClients = args.numClients;
var probeInterval = args.probeInterval;
var benchDuration = args.benchDuration * 1000;
var host = 'http://' + args.host + ':' + args.port;
var resultsFile = __dirname + '/results.json';

http.globalAgent.maxSockets = Infinity;

console.log('Running benchmark, ' + numClients + ' clients, ' + args.benchDuration + ' seconds');

var responseTimes = [];
var responseTimer = setInterval(getResponseTime, probeInterval);

var sysLoad = [], memUsage = [];
var sysLoadTimer  = setInterval(getSystemLoad, probeInterval);
var pushDiffTimes = [];

var clients = [], sampleRate = Math.floor(numClients / 20), client;
for (var i = 0; i < numClients; i++) {
    client = new EventSource(host + '/sse');
    clients.push(client);

    if (i === 0 || i % sampleRate === 0 || i === numClients) {
        client.onmessage = onMsgReceived;
    }
}

function getResponseTime() {
    var start = Date.now();
    var req = http.get(host + '/sse', function() {
        req.abort();
        responseTimes.push(Date.now() - start);

        if (responseTimes.length >= (benchDuration / probeInterval)) {
            clearInterval(responseTimer);
            clearInterval(sysLoadTimer);
            printReport();
        }
    });
}

function getSystemLoad() {
    sysLoad.push(os.loadavg()[0]);
    memUsage.push(os.totalmem() - os.freemem());
}

function printReport() {
    clients.forEach(function(client) {
        client.close();
    });

    var total = 0, totalDiff = 0;
    responseTimes.forEach(function(duration) {
        total += duration;
    });

    pushDiffTimes.forEach(function(diff) {
        totalDiff += diff;
    });

    var loadAvg = 0, memTotal = 0;
    sysLoad.forEach(function(load) {
        loadAvg += load;
    });

    memUsage.forEach(function(mem) {
        memTotal += mem;
    });

    var avg = parseInt(total / responseTimes.length, 10),
        max = Math.max.apply(null, responseTimes),
        min = Math.min.apply(null, responseTimes),
        avgDeliv = parseInt(totalDiff / pushDiffTimes.length, 10),
        maxDeliv = Math.max.apply(null, pushDiffTimes),
        minDeliv = Math.max(0, Math.min.apply(null, pushDiffTimes)),
        avgSysLoad = (loadAvg / sysLoad.length).toFixed(3),
        maxSysLoad = Math.max.apply(null, sysLoad),
        minSysLoad = Math.max(0, Math.min.apply(null, sysLoad)),
        avgMemory = parseInt(memTotal / memUsage.length, 10),
        maxMemory = Math.max.apply(null, memUsage),
        minMemory = Math.max(0, Math.min.apply(null, memUsage)),
        resData  = '[]';

    if (fs.existsSync(resultsFile)) {
        resData = fs.readFileSync(resultsFile, { encoding: 'utf8' });
    }
    
    var results;
    try {
        results = JSON.parse(resData) || [];
    } catch (e) {
        results = [];
    }

    var newIndex = results.push({
        profile: args.profileName,
        timestamp: (new Date()).toISOString().replace(/T/, ' ').substr(0, 19),
        duration: args.benchDuration + 's',
        avgResponse: avg + 'ms',
        maxResponse: max + 'ms',
        minResponse: min + 'ms',
        avgDelivery: avgDeliv + 'ms',
        maxDelivery: maxDeliv + 'ms',
        minDelivery: minDeliv + 'ms',
        avgMemory: avgMemory,
        maxMemory: maxMemory,
        minMemory: minMemory,
        avgSysLoad: avgSysLoad,
        maxSysLoad: maxSysLoad,
        minSysLoad: minSysLoad
    });

    fs.writeFileSync(resultsFile, JSON.stringify(results, null, 4), {
        encoding: 'utf8'
    });

    results[newIndex - 1].profile += ' (current)';

    var table = new Table({
        head: [
            'Profile', 'Date', 'Duration', 'Avg resp', 'Max resp', 'Min resp',
            'Avg dlv', 'Max del', 'Min del', 'Load avg', 'Avg. mem' 
        ]
    });

    results.forEach(function(result) {
        table.push([
            result.profile,
            result.timestamp,
            result.duration,
            result.avgResponse,
            result.maxResponse,
            result.minResponse,
            result.avgDelivery,
            result.maxDelivery,
            result.minDelivery,
            result.avgSysLoad,
            humanize.filesize(result.avgMemory)
        ]);
    });

    console.log(table.toString());
}

function onMsgReceived(e) {
    pushDiffTimes.push(Date.now() - e.data);
}