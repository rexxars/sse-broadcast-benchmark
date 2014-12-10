require 'eventmachine'
require 'em-http-server'

class SSEServer < EventMachine::HttpServer::Server
  @@connectionCount = 0
  @@connections = []

  def self.initialize_timer
    EventMachine::PeriodicTimer.new(1) do
      @@connections.each do |connection|
        time = Time.now.to_i * 1000

        connection.send_data "data: #{time.to_s}\n\n"
      end
    end
  end

  def initialize
    super

    @connection = nil
  end

  def process_http_request
    case @http_request_uri
    when '/connections'
      handle_connections
    when '/sse'
      handle_sse
    else
      handle_404
    end
  end

  def unbind
    unless @connection.nil?
      @@connectionCount -= 1
      @@connections.delete(@connection)
    end
  end

  def http_request_errback(e)
    puts e.inspect
  end

  private
  def handle_connections
    response = EM::DelegatedHttpResponse.new(self)
    response.status = 200
    response.content_type 'text/plain'
    response.headers['Cache-Control'] = 'no-cache'
    response.headers['Connection'] = 'close'
    response.content = @@connectionCount
    response.send_response
  end

  def handle_sse
    response = EventMachine::DelegatedHttpResponse.new(self)
    response.status = 200
    response.content_type 'text/event-stream'
    response.headers['Cache-Control'] = 'no-cache'
    response.headers['Connection'] = 'keep-alive'
    response.keep_connection_open

    send_headers(response)

    response.send_data ":ok\n\n"

    @connection = response

    @@connectionCount += 1
    @@connections.push(response)
  end

  def handle_404
    response = EM::DelegatedHttpResponse.new(self)
    response.status = 404
    response.content_type 'text/plain'
    response.content = 'File not found'
    response.send_response
  end

  def send_headers(response)
    raise "sent headers already" if @sent_headers
    @sent_headers = true

    ary = []
    ary << "HTTP/1.1 #{response.status || 200} #{response.status_string || '...'}\r\n"

    response.headers.each do |name, value|
      ary << "#{name}: #{value}\r\n"
    end

    ary << "\r\n"

    send_data ary.join
  end
end

EventMachine::run do
  port = ARGV[0] || 1942

  EventMachine::start_server "127.0.0.1", port, SSEServer

  SSEServer.initialize_timer

  puts "Listening on http://127.0.0.1:#{port}/"
end
