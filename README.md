# sse-broadcast-benchmark

An attempt to see how different languages/implementations handle broadcasting messages to a large number of open connections

## What

I wanted to make a thing where you could connect a large number of clients to a server and broadcast messages to all the clients in ~realtime. Websockets are cool, but there's a bit of overhead with regards to HTTP upgrades and whatnot, and there's really no point for two-way communication in my case.

So, this thing uses SSE. SSE (Server-Sent Event/EventSource) is a really simple protocol: basically just a regular HTTP request, which you respond to with a `Content-Type` of `text/event-stream`. Instead of closing the connection, you write data to the open connections, each message in the form of a `data: <someData>\n\n`-chunk.

## Rules

The server has five tasks to perform:

1. Respond to `/sse` with an open HTTP connection. Each connection should receive the following bytes when it opens:

    ```
    :ok\n\n
    ```

    And the response headers for the request should include:

    ```
    Content-Type: text/event-stream
    Cache-Control: no-cache
    Access-Control-Allow-Origin: *
    ```

2. Respond to `/connections` with the total number of active, open connections. Headers for this response should include:

    ```
    Content-Type: text/plain
    Cache-Control: no-cache
    Access-Control-Allow-Origin: *
    Connection: close
    ```

3. Respond to `POST /broadcast` with a `202`. The raw body should be broadcasted as a message to all connected clients. Headers should include:

    ```
    Content-Type: text/plain
    Cache-Control: no-cache
    Access-Control-Allow-Origin: *
    Connection: close
    ```

4. Respond to HTTP `OPTION` requests against `/connections` and `/sse` with a `204`. Headers should include:

    ```
    Access-Control-Allow-Origin: *
    Connection: close
    ```

5. Respond to all other HTTP requests with a `404`.
6. Every 1000 milliseconds, the server should broadcast a message on all open connections. The message should be dispatched from a central location - imagine the message is received through an HTTP POST request and should be broadcasted to all clients (ie. we don't know the message content in advance).The data should simply be the current unix timestamp, including milliseconds. Example packet would be:

    ```
    data: 1418071333619\n\n
    ```

## Testing

I've written a simple test to ensure the six basic rules above work as expected. It's written in node.js. Run it by installing it's dependencies (`npm install`), then doing `npm test`. If the server is not listening on port `1942`, run `node test.js <portNumber>`.

## Additional guidelines

* Have the implementation listen on port 1942 by default. Allowing a different port to be specified is a bonus feature, but not required.
* Feel free to print a status to stdout when the server starts
* Don't print anything when clients connect/disconnect or similar

## Contributing

Feel free! Both optimizations, tips and new implementations are welcomed. I'd prefer if the tests were written as plain and simple as possible. See `node.js` for the sample implementation.

If you're doing a new implementation, create a folder named after the language you're writing it in, and if using some specific framework, append it to the name (eg. `node.js-express`).

Also, make sure you include a `README.md`. It should explain:

  - How to install any dependencies you might need
  - How to run it
  - A list of contributors

## License

All code in this repository is MIT-licensed. See `LICENSE`.
