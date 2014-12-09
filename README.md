# sse-broadcast-benchmark

An attempt to see how different languages/implementations handle broadcasting messages to a large number of open connections

## What

I wanted to make a thing where you could connect a large number of clients to a server and broadcast messages to all the clients in ~realtime. Websockets are cool, but there's a bit of overhead with regards to HTTP upgrades and whatnot, and there's really no point for two-way communication in my case.

So, this thing uses SSE. SSE (Server-Sent Event/EventSource) is a really simple protocol: basically just a regular HTTP request, which you respond to with a `Content-Type` of `text/event-stream`. Instead of closing the connection, you write data to the open connections, each message in the form of a `data: <someData>\n\n`-chunk.

## Rules

The server has four tasks to perform:

1. Respond to `/sse` with an open HTTP connection. Each connection should receive the following bytes when it opens:
    
    ```
    :ok\n\n
    ```

    And the response headers for the request should include:
    
    ```
    Content-Type: text/event-stream
    Cache-Control: no-cache
    Connection: keep-alive
    ```
    
2. Respond to `/connections` with the total number of active, open connections. Headers for this response should include:
    
    ```
    Content-Type: text/plain
    Cache-Control: no-cache
    Connection: close
    ```

3. Respond to all other HTTP requests with a `404`.
4. Every 1000 milliseconds, the server should broadcast a message on all open connections. The message should be dispatched from a central location - imagine the message is received through an HTTP POST request and should be broadcasted to all clients (ie. we don't know the message content in advance).The data should simply be the current unix timestamp, including milliseconds. Example packet would be:
    
    ```
    data: 1418071333619\n\n
    ```

## Testing

I've written a simple test to ensure the four basic rules above work as expected. It's written in node.js. Run it by doing `npm test`, or if the server is not listening on port `1942`, run `node test.js <portNumber>`.

## Contributing

Feel free! Both optimizations, tips and new implementations are welcomed. I'd prefer if the tests were written as plain and simple as possible. See `node.js` for the sample implementation.

If you're doing a new implementation, create a folder named after the language you're writing it in, and if using some specific framework, append it to the name (eg. `node.js-express`).

## License

All code in this repository is MIT-licensed. See `LICENSE`.