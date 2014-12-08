'use strict';

var net  = require('net');
var test = require('tape');
var http = require('http');
var port = process.argv[2] || 1942;
var url  = 'http://127.0.0.1:' + port;

var conn = net.connect(port).on('error', function(e) {
    if (e.code === 'ECONNREFUSED') {
        console.error('=====================================================');
        console.error(' ERROR:');
        console.error(' HTTP server doesn\'t seem to be running on port ' + port);
        console.error('=====================================================\n');
        process.exit(1);
    }
}).on('connect', function() {
    conn.end();
    runTests();
});

function runTests() {

    test('gives 404 on undefined resource', function(t) {
        t.plan(1);
        var req = http.get(url + '/undefined', function(res) {
            t.equal(res.statusCode, 404, '/undefined should give a 404');
            req.abort();
        });
    });

    test('gives a connection count on /connections', function(t) {
        var req = http.get(url + '/connections', function(res) {
            t.equal(res.statusCode, 200, '/connections should give a 200');
            t.equal(res.headers['content-type'], 'text/plain', 'type should be text/plain');
            t.equal(res.headers['cache-control'], 'no-cache', 'cache-control should be no-cache');
            t.equal(res.headers['connection'], 'close', 'connection should be close');

            var body = '';
            res.on('data', function(chunk) {
                body += chunk;
            });

            req.on('close', function() {
                t.ok(body.toString().match(/^\d+$/), 'connection count should be an integer');
                t.end();
            });
        });
    });

    test('gives an SSE-connection on /', function(t) {
        var timeout = setTimeout(function() {
            req.abort();
            t.fail('did not receive :ok within 250ms');
        }, 250);

        var req = http.get(url + '/', function(res) {
            t.equal(res.statusCode, 200, '/connections should give a 200');
            t.equal(res.headers['content-type'], 'text/event-stream', 'type should be text/event-stream');
            t.equal(res.headers['cache-control'], 'no-cache', 'cache-control should be no-cache');
            t.equal(res.headers['connection'], 'keep-alive', 'connection should be keep-alive');
        
            var body = '';
            res.on('data', function(chunk) {
                body += chunk.toString();

                if (body.indexOf('\n\n') !== -1) {
                    t.equal(body.indexOf(':ok\n\n'), 0, 'should send :ok message as first bytes');
                    t.end();

                    req.abort();
                    clearTimeout(timeout);
                }
            });
        });
    });

    test('sends timestamps every second', function(t) {
        var body = '';
        var req = http.get(url + '/', function(res) {
            res.on('data', function(chunk) {
                body += chunk.toString();
            });
        });

        setTimeout(function() {
            req.abort();

            var dataPattern = /data:\s(\d{13})\n\n/g;

            var match, matches = [];
            while (match = dataPattern.exec(body)) {
                matches.push(parseInt(match[1], 10));
            }

            t.equal(matches.length, 3, 'should receive 3 messages');

            var threshold = Date.now() - 4000, now = Date.now();
            for (var i = 0; i < matches.length; i++) {
                t.ok(matches[i] > threshold, 'timestamp should not be older than 4 seconds');
                t.ok(matches[i] <= now, 'timestamp should be older than current timestamp');
            }

            t.end();
        }, 3150);
    });

}