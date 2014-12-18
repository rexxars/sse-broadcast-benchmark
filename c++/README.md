# sse-broadcast, C++ implementation

## Running

Simply execute `server`:

```
$ ./server
```

## How to deal with many clients on Linux?

```
echo 20000000 > /proc/sys/fs/nr_open
ulimit -n 20000000
sysctl -w net.ipv4.ip_local_port_range="500   65535"
sysctl -w net.ipv4.tcp_rmem="1024   4096   16384"
sysctl -w net.ipv4.tcp_wmem="1024   4096   16384"
sysctl -w net.ipv4.tcp_moderate_rcvbuf="0"
```

## Compiling

Ensure you have libboost installed, then run `make`

## Contributors

- [Einar Otto Stangvik](https://github.com/einaros)
- [Espen Hovlandsdal](https://github.com/rexxars)
