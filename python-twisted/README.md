# sse-broadcast, python-twisted implementation

## Installing

The python-twisted implementation depends on Python and Twisted. If you're running ubuntu or debian, twisted is available as a package called `python-twisted`.

```bash
$ sudo apt-get install python-twisted
```

## Running

In order to start twisted server as a daemon execute

```bash
$ twistd -y server.py
```

If you instead want to run it in the foreground, add an n

```bash
$ twistd -ny server.py
```

## Notes

Port number is 1942

## Contributors

- [Kristoffer Braband](https://github.com/kbrabrand)
