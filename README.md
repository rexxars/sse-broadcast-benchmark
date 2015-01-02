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

3. Respond to HTTP `OPTIONS` requests against `/connections` and `/sse` with a `204`. Headers should include:

    ```
    Access-Control-Allow-Origin: *
    Connection: close
    ```

4. Respond to HTTP `POST` requests against `/broadcast` with a `202`. The server should take the entire request body and broadcast the data found to all clients. For example, when sending a POST-request with the following body:

   ```
   this is a message - broadcast it!
   ```

The clients subscribing to `/sse` should receive:

   ```
   data: this is a message - broadcast it!
   ```

Requests without a Content-Length *can* be discarded, but in this case the server needs to respond with a `411`. If the server handles chunked transfer encoding, feel free to broadcast the message as one normally would.

5. Respond to all other HTTP requests with a `404`.

## Testing

I've written a simple test to ensure the five basic rules above work as expected. It's written in node.js. Run it by installing it's dependencies (`npm install`), then doing `npm test`. If the server is not listening on port `1942`, run `node test.js <portNumber>`.

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