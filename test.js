'use strict';

var net  = require('net');
var test = require('tape');
var http = require('http');
var port = process.argv[2] || 1942;
var host = '127.0.0.1';
var url  = 'http://' + host + ':' + port;
var parseUrl = require('url').parse;

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

    test('gives a 202 on /broadcast POST', function(t) {
        t.plan(3);

        var options = parseUrl(url + '/broadcast');
        options.method = 'POST';


        var req = http.request(options, function(res) {
            t.equal(res.headers['content-type'], 'text/plain', 'type should be text/plain');
            t.equal(res.headers['cache-control'], 'no-cache', 'cache-control should be no-cache');
            t.equal(res.headers['connection'], 'close', 'connection should be close');
        });

        req.write('foobar');
        req.end();
    });

    test('sends correct cors-header', function(t) {
        t.plan(8);

        http.get(url + '/connections', function(res) {
            t.equal(res.headers['access-control-allow-origin'], '*', '/connections should send cors header');
        });

        var req = http.get(url + '/sse', function(res) {
            t.equal(res.headers['access-control-allow-origin'], '*', '/sse should send cors header');
            req.abort();
        });

        var options = parseUrl(url + '/connections');
        options.method = 'OPTIONS';

        http.request(options, function(res) {
            t.equal(res.statusCode, 204, 'OPTIONS /connections should give a 204');
            t.equal(res.headers['access-control-allow-origin'], '*', 'OPTIONS /connections should send cors header');
            t.equal(res.headers['connection'], 'close', 'OPTIONS /connections should send `connection: close`');
        }).end();

        options.path = '/sse';
        http.request(options, function(res) {
            t.equal(res.statusCode, 204, 'OPTIONS /sse should give a 204');
            t.equal(res.headers['access-control-allow-origin'], '*', 'OPTIONS /sse should send cors header');
            t.equal(res.headers['connection'], 'close', 'OPTIONS /sse should send `connection: close`');
        }).end();
    });

    test('gives an SSE-connection on /sse', function(t) {
        var timeout = setTimeout(function() {
            req.abort();
            t.fail('did not receive :ok within 250ms');
        }, 250);

        var req = http.get(url + '/sse', function(res) {
            t.equal(res.statusCode, 200, '/sse should give a 200');
            t.equal(res.headers['content-type'], 'text/event-stream', 'type should be text/event-stream');
            t.equal(res.headers['cache-control'], 'no-cache', 'cache-control should be no-cache');

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
        var req = http.get(url + '/sse', function(res) {
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

            t.ok(matches.length >= 3, 'should receive 3 or more messages');

            var threshold = Date.now() - 4000, now = Date.now();
            for (var i = 0; i < matches.length; i++) {
                t.ok(matches[i] > threshold, 'timestamp should not be older than 4 seconds');
                t.ok(matches[i] <= now, 'timestamp should be older than current timestamp');
            }

            t.end();
        }, 3150);
    });

}