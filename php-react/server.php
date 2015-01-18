#!/usr/bin/env php
<?php
require __DIR__ . '/vendor/autoload.php';

$args = getArgs();
$port = $args['port'];

$connections = [];
$numConnections = 0;

$app = function ($request, $response) use (&$connections, &$numConnections) {
    if ($request->getMethod() === 'OPTIONS') {
        return handleCors($request, $response);
    }

    switch ($request->getPath()) {
        case '/broadcast':
            return handleBroadcast($request, $response);
        case '/connections':
            return handleConnectionCount($request, $response);
        case '/sse':
            return handleSse($request, $response);
        default:
            return handle404($request, $response);
    }
};

function handleBroadcast($request, $response) {
    $body = '';
    $request->on('data', function($chunk) use (&$body) {
        $body .= $chunk;
    });

    $request->on('end', function() use (&$body) {
        broadcast($body);
    });

    $response->writeHead(202);
    $response->end();
}

function handleCors($request, $response) {
    $response->writeHead(204, [
        'Cache-Control' => 'no-cache',
        'Connection'    => 'close',
        'Access-Control-Allow-Origin' => '*'
    ]);
    $response->end();
}

function handleConnectionCount($request, $response) {
    global $numConnections; // lol, iknowrite?

    $response->writeHead(200, [
        'Content-Type'  => 'text/plain',
        'Cache-Control' => 'no-cache',
        'Connection'    => 'close',
        'Access-Control-Allow-Origin' => '*'
    ]);
    $response->end($numConnections);
}

function handle404($request, $response) {
    $response->writeHead(404);
    $response->end('File not found');
}

function handleSse($request, $response) {
    global $numConnections; // lol, iknowrite?
    global $connections;    // lol, iknowrite?

    $numConnections++;

    $response->writeHead(200, [
        'Content-Type'  => 'text/event-stream',
        'Cache-Control' => 'no-cache',
        'Connection'    => 'keep-alive',
        'Access-Control-Allow-Origin' => '*'
    ]);

    $response->write(":ok\n\n");

    $index = array_push($connections, $response);
    $request->on('end', function() use (&$numConnections, &$connections, $index) {
        $numConnections--;
        unset($connections[$index]);
    });
}

function getArgs() {
    global $argv;

    $args = ['port' => 1942];

    for ($i = 0; $i < count($argv); $i++) {
        $arg = $argv[$i];

        if ($arg === '--port') {
            $args['port'] = $argv[++$i];
        }
    }

    return $args;
}

function broadcast($data) {
    global $connections;

    foreach ($connections as $response) {
        $response->write('data: ' . $data . "\n\n");
    }
}

$loop = React\EventLoop\Factory::create();
$socket = new React\Socket\Server($loop);
$http = new React\Http\Server($socket);
$http->on('request', $app);

$socket->listen($port);

echo 'Listening on http://127.0.0.1:' . $port . '/' . PHP_EOL;
$loop->run();
