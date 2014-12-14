# sse-broadcast, C implementation

## Installing

The only required library is the POSIX Thread library (pthread).

## Running

Simply execute `server`:

```
$ ./server
```

## Compiling

```
$ gcc -pthread server.c -o server
```

## Notes

The server will start 8 SSE-worker threads by default and balance the load equally between them.
You can configure how many SSE worker threads to run by editing NUM_THREADS.

Port number is by default `1942`, but could be changed by LISTEN_PORT in the source code and recompile it.

If you want more logging change the LOGLEVEL to LOG_DEBUG.

## Contributors

- [Ole Fredrik Skudsvik](https://github.com/olesku)
