# sse-broadcast, PHP w/React implementation

## Installing

- Make sure [composer](https://getcomposer.org/download/) is installed
- Run `composer install`

## Running

Simply execute server.php:

```
$ ./server.php
```

## Notes

The server runs in a single process. To use multiple processes, you'll have to start multiple instances with different port numbers and put a load balancer in front of them.

Port number is by default `1942`, but can be changed with `--port <portNumber>`.

## Contributors

- [Espen Hovlandsdal](https://github.com/rexxars)


