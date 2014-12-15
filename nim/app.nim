import server, asyncdispatch, asyncnet, strtabs, sequtils, times, os, strutils

var previousUpdateTime = toInt(epochTime() * 1000)

var clients: PClients
new clients
clients[] = @[]

proc handleCORS(req: Request) {.async.} =
  let headers = {"Access-Control-Allow-Origin": "*", "Connection": "close"}

  await req.respond(Http204, "", headers.newStringTable())

proc handleConnections(req: Request, clients: PClients) {.async.} =
  let clientCount = clients[].len
  let headers = {"Content-Type": "text/plain", "Access-Control-Allow-Origin": "*",
                 "Cache-Control": "no-cache", "Connection": "close"}

  await req.respond(Http200, $clientCount, headers.newStringTable())
  req.client.close()

proc handle404(req: Request) {.async.} =
  let headers = {"Content-Type": "text/plain", "Connection": "close"}

  await req.respond(Http404, "File not found", headers.newStringTable())
  req.client.close()

proc handleSSE(req: Request, clients: PClients) {.async.} =
  let headers = {"Content-Type": "text/event-stream", "Access-Control-Allow-Origin": "*",
                 "Cache-Control": "no-cache", "Connection": "keep-alive"}

  await req.client.send("HTTP/1.1 200 OK\c\L")
  await req.sendHeaders(headers.newStringTable())
  await req.client.send("\c\L:ok\n\n")
  clients[].add req.client

proc requestCallback(req: Request, clients: PClients) {.async.} =
  if req.reqMethod == "OPTIONS":
    asyncCheck handleCORS(req)
  else:
    case req.url.path
    of "/connections": asyncCheck handleConnections(req, clients)
    of "/sse": asyncCheck handleSSE(req, clients)
    else: asyncCheck handle404(req)

proc pingClients(clients: PClients) {.async.} =
  let currentTime = toInt(epochTime() * 1000)

  if currentTime - previousUpdateTime < 1000: return

  for client in clients[]:
    if not client.closed:
      asyncCheck client.send("data: " & $currentTime & "\n\n")

  previousUpdateTime = toInt(epochTime() * 1000)

proc checkClients() =
  clients[] = clients[].filterIt(it.closed != true)

when isMainModule:
  var port: int

  if paramCount() == 1:
    try: port = paramStr(1).parseInt()
    except EInvalidValue: port = 1942
  else: port = 1942

  let httpServer = newAsyncHttpServer(true)

  asyncCheck httpServer.serve(Port(port), clients, requestCallback)

  echo "Listening on http://127.0.0.1:" & $port

  while true:
    checkClients()
    asyncCheck pingClients(clients)
    poll()
