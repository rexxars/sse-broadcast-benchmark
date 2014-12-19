#!/bin/sh
// 2>/dev/null; exec "$(command -v nodejs || command -v node)" "$0" "$@"

'use strict';

var net = require('net');
var cluster = require('cluster');
var numCPUs = require('os').cpus().length;
var statusCodes = { 200: 'OK', 204: 'No Content', 404: 'File not found', 400: 'Client error' };

var args = getArgs();
var port = args.port || 1942;
var clients = [], numClients = 0;

setInterval(function() {
    broadcast(Date.now());
}, 1000);

if (args.cluster && cluster.isMaster) {
    console.log('Forking ' + numCPUs + ' processes');
    for (var i = 0; i < numCPUs; i++) {
        cluster.fork();
    }
} else {
    net.createServer(function(c) {
        var buffer = '';
        var parseData = function(chunk) {
            buffer += chunk;

            for (var i = 0; i < chunk.length; i++) {
                if (chunk[i] === 10) {
                    c.removeListener('data', parseData);
                    return handleRequest(buffer.slice(0, i - 1).toString(), c);
                }
            }
        };

        c.on('data', parseData);
    }).listen(port, '127.0.0.1', function() {
        console.log('Listening on http://127.0.0.1:' + port + '/');
    });
}

function handleRequest(header, res) {
    var parts = header.split(' ', 3);
    if (parts.length < 3) {
        writeClientError(res);
    } else if (parts[0] === 'OPTIONS') {
        writeCorsOptions(res);
    } else if (parts[1].indexOf('/connections') === 0) {
        writeConnectionCount(res);
    } else if (parts[1] === '/sse') {
        initSseClient(res);
    } else {
        write404(res);
    }
}

function writeHead(c, code, headers) {
    c.write('HTTP/1.1 ' + code + ' ' + statusCodes[code] + '\r\n');
    for (var header in headers) {
        c.write(header + ': ' + headers[header] + '\r\n');
    }

    c.write('\r\n');
}

function writeConnectionCount(res) {
    writeHead(res, 200, {
        'Content-Type': 'text/plain',
        'Cache-Control': 'no-cache',
        'Access-Control-Allow-Origin': '*',
        'Connection': 'close'
    });
    res.write('' + numClients);
    res.end();
}

function writeCorsOptions(res) {
    writeHead(res, 204, {
        'Access-Control-Allow-Origin': '*',
        'Connection': 'close'
    });
    res.end();
}

function writeClientError(res) {
    writeHead(res, 400, {
        'Content-Type': 'text/plain',
        'Connection': 'close'
    });
    res.end();
}

function write404(res) {
    writeHead(res, 404, {
        'Content-Type': 'text/plain',
        'Connection': 'close'
    });
    res.write('File not found');
    res.end();
}

function initSseClient(res) {
    res.setTimeout(Infinity);
    res.setNoDelay(true);
    writeHead(res, 200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        'Access-Control-Allow-Origin': '*',
        'Connection': 'keep-alive'
    });

    res.write(':ok\n\n');

    var clientIndex = clients.push(res) - 1;
    numClients++;

    // We could alternatively slice it out.
    // Not sure what is more optimal here - benchmark!
    var removeClient = (function(index) {
        return function() {
            clients[index] = null;
            numClients--;
        };
    })(clientIndex);

    res.on('end', removeClient);
}

function broadcast(data) {
    var i = clients.length;
    while (i--) {
        if (clients[i]) {
            clients[i].write('data: ' + data + '\n\n');
        }
    }
}

function getArgs() {
    var args = {}, arg;

    for (var i = 2; i < process.argv.length; i++) {
        arg = process.argv[i];

        if (arg === '--port') {
            args.port = process.argv[++i];
        } else if (arg === '--cluster') {
            args.cluster = true;
        } else if (arg === '--help') {
            showHelp();
            process.exit(0);
        }
    }

    return args;
}

function showHelp() {
    console.log('sse-broadcast node.js implementation\n');

    console.log('  --port <portnum>\n');
    console.log('  Set port number to set up HTTP server on. Default: 1942\n');

    console.log('  --cluster\n');
    console.log('  Enables the cluster module, forking a number of processes equal to the number of CPUs.\n');
}