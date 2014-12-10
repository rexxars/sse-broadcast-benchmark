from bottle import Bottle, response, run

app = Bottle()

@app.get('/connections')
def connections():
    response.set_header('Content-Type', 'text/plain')
    response.set_header('Cache-Control', 'no-cache')
    response.set_header('Connection', 'close')

    return "%d" % app.get_connection_count()

wsgi_app = app

if __name__ == '__main__':
    run(app, host='localhost', port=8005)