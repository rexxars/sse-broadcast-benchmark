import sys
import argparse

# import SSE module

# Importing epoll reactor ans setting it up
from twisted.python import log
from twisted.internet import epollreactor

epollreactor.install()

from twisted.web import server, resource

# Import the reactor singleton
from twisted.internet import reactor

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
        request.setHeaders('Content-Type', 'text/plain')
        request.setHeaders('Cache-Control', 'no-cache')
        request.setHeaders('Access-Control-Allow-Origin', '*')
        request.setHeaders('Connection', 'close')

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

    def add(self, request):
        self.subscribers.add(request);
        log.msg('Establishing connection..')

        # Set up removal of subscriber on disconnect
        d = request.notifyFinish()
        d.addBoth(self.remove, request)


    def remove(self, reason, subscriber):
        if subscriber in self.subscribers:
            log.msg('Removing connection..')
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



parser = argparse.ArgumentParser(description='SSE server')
parser.add_argument('-p', '--port', metavar='N', type=int, default=1942, help="Port number")

args = parser.parse_args();

root      = Root()
subscribe = Subscribe()

root.putChild('sse', subscribe)
root.putChild('connections', Connections(subscribe.get_subscribers_count))

site = server.Site(root)

reactor.listenTCP(args.port, site)

# Run the reactor
print 'Starting the reactor.'
reactor.run()
