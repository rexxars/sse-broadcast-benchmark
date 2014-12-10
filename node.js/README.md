# sse-broadcast, node.js implementation

## Installing

There are no dependencies apart from node.js itself.
Easiest way to install node.js is probably through [nvm](https://github.com/creationix/nvm).

## Running

Simply execute server.js:

```
$ ./server.js
```

## Notes

The server runs by default in a single process. Use `--cluster` to fork processes equal to the number of CPUs available.

Port number is by default `1942`, but can be changed with `--port <portNumber>`.

## Contributors

- [Espen Hovlandsdal](https://github.com/rexxars)


