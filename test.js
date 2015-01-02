'use strict';

var net  = require('net');
var test = require('tape');
var http = require('http');
var request = require('request');
var port = process.argv[2] || 1942;
var url  = 'http://127.0.0.1:' + port;
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

    test('sends messages received by POST to /broadcast', function(t) {
        var payload1 = '' + Date.now() + Math.random();
        var payload2 = '' + Date.now() + Math.random();
        var body = '', statusCode;
        var req = http.get(url + '/sse', function(res) {
            res.on('data', function(chunk) {
                body += chunk.toString();
            });
        });

        request({
            uri: url + '/broadcast',
            method: 'POST',
            body: payload1
        }, function(err, res) {
            t.equal(res.statusCode, 202, 'server should give 202 when posting to /broadcast');
        });

        request({
            uri: url + '/broadcast',
            method: 'POST',
            body: payload2
        }, function(err, res) {
            t.equal(res.statusCode, 202, 'server should give 202 when posting to /broadcast');
        });

        setTimeout(function() {
            req.abort();

            t.ok(body.indexOf('data: ' + payload1 + '\n\n') > -1, 'should receive broadcasted messages')
            t.ok(body.indexOf('data: ' + payload2 + '\n\n') > -1, 'should receive broadcasted messages')

            t.end();
        }, 150);
    });

    /*
    test('either discards or accepts chunked transfers (with no content-length)', function(t) {
        var sseBody = '';
        var sseReq = http.get(url + '/sse', function(res) {
            res.on('data', function(chunk) {
                sseBody += chunk.toString();
            });
        });

        var response = '';
        var conn = net.connect(port).on('error', function() {
            // Disregard errors, they're usually "connection is closed"-messages
        }).on('data', function(data) {
            response += data.toString();
        }).on('connect', function() {
            conn.write('POST /broadcast HTTP/1.1\r\n');
            conn.write('Host: localhost:' + port + '\r\n');
            conn.write('content-type: text/plain\r\n');
            conn.write('Transfer-Encoding: chunked\r\n');
            conn.write('\r\n');
            conn.write('6\r\n');
            conn.write('We are\r\n');

            setTimeout(function() {
                conn.write('9\r\n');
                conn.write(' winners!');
                conn.write('0\r\n\r\n')
                conn.end();
            }, 50);
        }).on('end', function() {
            setTimeout(function() {
                sseReq.abort();

                var match = response.match(/^HTTP\/\d\.\d\s(\d+)/),
                    resCode = match ? match[1] | 0 : null,
                    msgRcvd = sseBody.indexOf('data: We are winners!') > -1;

                if (!resCode) {
                    t.fail('Failed to get a proper HTTP response from /broadcast');
                } else if (resCode === 202) {
                    t.ok(msgRcvd, '"We are winners!"-message should be broadcasted')
                } else {
                    t.ok(resCode === 411, 'Response code should be 411 if rejected');
                }

                t.end();
            }, 150);
        });
    });
    */

}