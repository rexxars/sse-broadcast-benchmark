from twisted.web import server, resource

class Root(resource.Resource):
    """
    Root resource; serve
    """
    def getChild(self, name, request):
        if name == '':
            return self

        return resource.Resource.getChild(self, name, request)

    def render_GET(self, request):
        return '<img src="http://media.tumblr.com/9e64e7067cd7b0d0aabd14ef265bf468/tumblr_inline_nbj2vnMNGW1s0m7nr.gif"/>';

class Connections(resource.Resource):
    """
    Connections resource serves number of active connections
    """
    isLeaf = True

    def __init__(self, subscribers_count):
        self.subscribers_count = subscribers_count

    def render_GET(self, request):
        request.setResponseCode(200)
        request.setHeader('Content-Type', 'text/plain')
        request.setHeader('Cache-Control', 'no-cache')
        request.setHeader('Access-Control-Allow-Origin', '*')
        request.setHeader('Connection', 'close')

        return "%s" % self.subscribers_count()

    def render_OPTIONS(self, request):
        request.setHeader('Access-Control-Allow-Origin', '*')
        request.setHeader('Connection', 'close')

        return "";

class Subscribe(resource.Resource):
    """
    Subscribe resource sets up new event stream connections
    """
    isLeaf = True

    def __init__(self):
        self.subscribers = set()

    def get_subscribers_count(self):
        return len(self.subscribers)

    def add(self, request):
        self.subscribers.add(request);

        # Set up removal of subscriber on disconnect
        d = request.notifyFinish()
        d.addBoth(self.remove, request)

    def publish_to_all(self, data):
        for subscriber in self.subscribers:
            for line in data:
                subscriber.write("data: %s\n" % line)

            # NOTE: the last CRLF is required to dispatch the event at the client
            subscriber.write("\n\n")

    def remove(self, reason, subscriber):
        if subscriber in self.subscribers:
            self.subscribers.remove(subscriber)

    def render_GET(self, request):
        request.setHeader('Content-Type', 'text/event-stream')
        request.setHeader('Cache-Control', 'no-cache')
        request.setHeader('Access-Control-Allow-Origin', '*')
        request.setResponseCode(200)

        # Add connection
        self.add(request)

        request.write(':ok\n\n')

        return server.NOT_DONE_YET

    def render_OPTIONS(self, request):
        request.setHeader('Access-Control-Allow-Origin', '*')
        request.setHeader('Connection', 'close')

        return "";

class Publish(resource.Resource):
    """
    Publish data to subscribers
    """
    isLeaf = True

    def __init__(self, publish_to_all):
        self.publish_to_all = publish_to_all

    def render_POST(self, request):
        request.setHeader('Content-Type', 'text/plain')
        request.setHeader('Cache-Control', 'no-cache')
        request.setHeader('Access-Control-Allow-Origin', '*')
        request.setResponseCode(202)

        return ""

    def render_OPTIONS(self, request):
        request.setHeader('Access-Control-Allow-Origin', '*')
        request.setHeader('Connection', 'close')

        return "";

