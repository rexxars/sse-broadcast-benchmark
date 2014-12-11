# sse-broadcast, python-twisted implementation

## Installing

The python-twisted implementation depends on Python, Twisted and a few python modules. If you're running ubuntu or debian, twisted is available as a package called `python-twisted`.

```bash
$ sudo apt-get install pyton-twisted
```

Additionally, you need to `bottle` and `crotchet` installed. The easiest way is by using pip:

```bash
$ sudo pip install bottle crotchet
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

The solution is based on the proposed [Twisted SSE implementation](http://orestis.gr/blog/2014/02/03/wsgi-twisted-and-server-sent-events/) by [Orestis Markou](https://gist.github.com/orestis).

Port number is 1942

## Contributors

- [Kristoffer Braband](https://github.com/kbrabrand)
