#!/bin/sh
// 2>/dev/null; exec "$(command -v nodejs || command -v node)" "$0" "$@"

'use strict';

var http = require('http');
var port = process.argv[2] || 1942;
var clients = [], numClients = 0;

http.createServer(function(req, res) {
    if (req.url.indexOf('/connections') === 0) {
        writeConnectionCount(res);
    } else if (req.url === '/') {
        initClient(req, res);
    } else {
        write404(res);
    }
}).listen(port, '127.0.0.1', function() {
    console.log('Listening on http://127.0.0.1:' + port + '/');
});

setInterval(function() {
    broadcast(Date.now());
}, 1000);

function writeConnectionCount(res) {
    res.writeHead(200, {
        'Content-Type': 'text/plain',
        'Cache-Control': 'no-cache',
        'Connection': 'close'
    });
    res.write('' + numClients);
    res.end();
}

function write404(res) {
    res.writeHead(400, {
        'Content-Type': 'text/plain',
        'Connection': 'close'
    });
    res.write('File not found');
    res.end();
}

function initClient(req, res) {
    req.socket.setTimeout(Infinity);
    req.socket.setNoDelay(true);
    req.socket.setKeepAlive(true);
    res.writeHead(200, {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
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

    // Not sure which of these are required - start with only close,
    // then work our way down if stuff crashes
    req.on('close',  removeClient);
    req.on('end',    removeClient);
    res.on('finish', removeClient);
}

function broadcast(data) {
    var i = clients.length;
    while (i--) {
        if (clients[i]) {
            clients[i].write('data: ' + data + '\n\n');
        }
    }

}