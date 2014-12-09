<?php
require __DIR__ . '/vendor/autoload.php';  

$port = (int) (isset($argv[2]) ? $argv[2] : 1942);

$connections = [];
$numConnections = 0;

$app = function ($request, $response) use (&$connections, &$numConnections) {
    switch ($request->getPath()) {
        case '/connections':
            return handleConnectionCount($request, $response);
        case '/sse':
            return handleSse($request, $response);
        default:
            return handle404($request, $response);
    }
};

function handleConnectionCount($request, $response) {
    global $numConnections; // lol, iknowrite?

    $response->writeHead(200, [
        'Content-Type'  => 'text/plain',
        'Cache-Control' => 'no-cache',
        'Connection'    => 'close',
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
    ]);

    $response->write(":ok\n\n");

    $index = array_push($connections, $response);
    $request->on('end', function() use (&$numConnections, &$connections, $index) {
        $numConnections--;
        unset($connections[$index]);
    });
}

$loop = React\EventLoop\Factory::create();
$socket = new React\Socket\Server($loop);
$http = new React\Http\Server($socket);
$http->on('request', $app);

$loop->addPeriodicTimer(1, function() use (&$connections) {
    foreach ($connections as $response) {
        $response->write('data: ' . floor(microtime(true) * 1000) . "\n\n");
    }
});

$socket->listen($port);

echo 'Listening on http://127.0.0.1:' . $port . '/' . PHP_EOL;
$loop->run();
