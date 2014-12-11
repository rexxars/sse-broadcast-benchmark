import crochet
crochet.setup()

from twisted.web import resource, server

def _format_sse(msg, event=None):
    data = []

    if event is not None:
        data.append("event: {}\n".format(event))

    for line in msg.splitlines():
        data.append("data: {}\n".format(line))

    data.append("\n")

    return "".join(data)

class SSEResource(resource.Resource):

    def __init__(self):
        resource.Resource.__init__(self)
        self._listeners = []

    def get_connection_count(self):
        return len(self._listeners);

    def add_listener(self, request):
        self._listeners.append(request)

        # Write initial ok
        request.write(':ok\n\n')

        request.notifyFinish().addBoth(self.remove_listener, request)

    def remove_listener(self, reason, listener):
        self._listeners.remove(listener)

    @crochet.run_in_reactor
    def broadcast_message(self, msg, event=None):
        self.broadcast_message_async(msg, event)

    def broadcast_message_async(self, msg, event=None):
        sse = _format_sse(msg, event)
        for listener in self._listeners:
            listener.write(sse)

    def render_GET(self, request):
        request.setHeader("Content-Type", "text/event-stream")
        request.setHeader("Cache-Control", "no-cache")
        request.setHeader("Connection", "keep-alive")

        self.add_listener(request)
        return server.NOT_DONE_YET
