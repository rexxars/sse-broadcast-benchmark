# crochet allows non-twisted apps to call twisted code
import crochet
crochet.no_setup()

from twisted.application import internet, service
from twisted.web import server as twisted_server, wsgi, static, resource
from twisted.internet import reactor, task
from twisted.python import threadpool

from time import time;

# boilerplate to get any WSGI app running under twisted
class WsgiRoot(resource.Resource):
    def __init__(self, wsgi_resource):
        resource.Resource.__init__(self)
        self.wsgi_resource = wsgi_resource

    def getChild(self, path, request):
        path0 = request.prepath.pop(0)
        request.postpath.insert(0, path0)
        return self.wsgi_resource

# create a twisted.web resource from a WSGI app.
def get_wsgi_resource(wsgi_app):
    pool = threadpool.ThreadPool()
    pool.start()

    # Allow Ctrl-C to get you out cleanly:
    reactor.addSystemEventTrigger('after', 'shutdown', pool.stop)
    wsgi_resource = wsgi.WSGIResource(reactor, pool, wsgi_app)

    return wsgi_resource

# broadcast timestamp - called as a twisted LoopingCall task
def emit_dummy_event(args):
    args['broadcast_func'](str(int(time() * 1000)));

def start():
    # create an SSE resource that is effectively a singleton
    from sse import SSEResource
    sse_resource = SSEResource()

    # attach its "broadcast_message" function to the WSGI app
    from app import wsgi_app
    wsgi_app.get_connection_count = sse_resource.get_connection_count

    # serve everything together
    root = WsgiRoot(get_wsgi_resource(wsgi_app)) # WSGI is the root
    root.putChild("sse", sse_resource) # serve the SSE handler

    # emit an event every second
    l = task.LoopingCall(emit_dummy_event, { "broadcast_func": sse_resource.broadcast_message })
    l.start(1.0)

    main_site = twisted_server.Site(root)
    server = internet.TCPServer(8005, main_site)
    application = service.Application("twisted_wsgi_sse_integration")
    server.setServiceParent(application)
    return application

application = start()
# run this using twistd -ny server.py