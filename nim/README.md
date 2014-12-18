# sse-broadcast, Nim implementation

## Installing

There are no dependencies apart from the devel version of Nim itself. See [nimrod - compiling](https://github.com/Araq/Nimrod#compiling).

## Running

Simply execute `app`:

```
$ ./app
```

## Compiling

```
$ nim c --threads:on -d:release app.nim
```

## Notes

I'm pretty new to Nim, so this might not be the most efficient implementation.

Port number is by default `1942`, but can be changed:

```
$ ./app <portNumber>
```

## Contributors

- [Espen Volden](https://github.com/voldern)