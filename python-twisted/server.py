# Set up the twisted epollreactor
from twisted.internet import epollreactor
epollreactor.install()

from twisted.python import threadpool
from twisted.application import service, internet
from twisted.internet import task, reactor
from twisted.web import server as twisted_server

from time import time;

def publish_timestamp(args):
    args['broadcast']([str(int(time() * 1000))]);

def start():
   # Import the module containing the resources
    import sse

    pool = threadpool.ThreadPool()
    pool.start()

    # Prepare the app root and subscribe endpoint
    root        = sse.Root()
    subscribe   = sse.Subscribe()
    broadcast   = sse.Broadcast(subscribe.publish_to_all)
    connections = sse.Connections(subscribe.get_subscribers_count)

    # Add sse and connections as children of root
    root.putChild('sse', subscribe)
    root.putChild('connections', connections)
    root.putChild('broadcast', broadcast)

    # Allow Ctrl-C to get you out cleanly:
    reactor.addSystemEventTrigger('after', 'shutdown', pool.stop)

    # emit an event every second
    l = task.LoopingCall(publish_timestamp, { "broadcast": subscribe.publish_to_all })
    l.start(1.0)

    site = twisted_server.Site(root)
    server = internet.TCPServer(1942, site)
    application = service.Application("twisted-sse")
    server.setServiceParent(application)
    return application

application = start()
# run this using twistd -ny server.py