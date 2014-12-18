from twisted.web import resource
from twisted.python import log

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
        request.setHeader('Content-Type', 'text/plain');

        return "%s" % self.subscribers_count()

class Subscribe(resource.Resource):
    """
    Subscribe resource sets up new event stream connections
    """
    isLeaf = True

    def __init__(self):
        self.subscribers = set()

    def get_subscribers_count(self):
        return len(self.subscribers)

    def render_GET(self, request):
        request.setHeader('Content-Type', 'text/event-stream; charset=utf-8')
        request.setHeader('Connection', 'keep-alive');
        request.setResponseCode(200)

        self.subscribers.add(request)

        request.write(':ok\n\n')

        #d = request.notifyFinish()
        #d.addBoth(self.removeSubscriber)

        return NOT_DONE_YET