#include <iostream>
#include <functional>
#include <map>
#include <list>
#include <string>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "interval.hpp"
#include "http_handler.hpp"
#include "sse_handler.hpp"

using boost::asio::ip::tcp;

class server
{
public:
  server(boost::asio::io_service& io_service, short port)
    : _acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
      _socket(io_service),
      _sse_clients()
  {
    init_handlers();
    do_accept();
  }
  
  void broadcast(const std::string& msg) {
    std::string full_msg = "data: " + msg + "\n\n";
    auto buf = boost::asio::buffer(full_msg, full_msg.length());
    auto i = std::begin(_sse_clients);
    while (i != std::end(_sse_clients)) {
      boost::asio::async_write(**i, buf,
        [this, i](boost::system::error_code ec, std::size_t) {
          // clients are lazily removed, meaning we won't bother
          // detecting disconnects until a failed write is flagged
          if (ec == boost::asio::error::eof ||
              ec == boost::asio::error::connection_reset ||
              ec == boost::asio::error::broken_pipe) {
            _sse_clients.erase(i);
            _sse_client_count -= 1;
          }
        });
      ++i;
    }
  }

private:
  typedef std::function<void(boost::system::error_code, std::size_t)> write_cb;
  
  // accept loop
  void do_accept() {
    _acceptor.async_accept(_socket,
        [this](boost::system::error_code ec) {
          if (!ec) {
            // pass control to the http handler
            std::make_shared<http_handler>(std::move(_socket), _handlers)->start();
          }
          do_accept();
        });
  }

  void write(std::shared_ptr<tcp::socket>& socket, const std::string& msg, write_cb cb) {
    boost::asio::async_write(*socket, boost::asio::buffer(msg, msg.length()), cb);
  }

  // minimal http request handlers
  void init_handlers() {
    _handlers = {
      {"GET /connections", [this](std::shared_ptr<tcp::socket>& socket) { 
        std::string msg = boost::str(boost::format("%d") % _sse_client_count);
        write(socket,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: " + boost::str(boost::format("%d") % msg.length()) + "\r\n"
          "Connection: close\r\n"
          "Cache-Control: no-cache\r\n"
          "\r\n" + msg,
          [this, socket](boost::system::error_code, std::size_t) {
            boost::system::error_code ec;
            socket->shutdown(tcp::socket::shutdown_both, ec);
          });
      }},
      {"GET /sse", [this](std::shared_ptr<tcp::socket>& socket) { 
        write(socket,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/event-stream\r\n"
          "Connection: keep-alive\r\n"
          "Cache-Control: no-cache\r\n"
          "\r\n",
          [this, socket](boost::system::error_code ec, std::size_t) {
            if (ec) {
              socket->shutdown(tcp::socket::shutdown_both, ec);
            }
            else {
              _sse_clients.push_back(std::move(socket));
              _sse_client_count += 1;
            }
          });
      }},
      {"POST /broadcast", [this](std::shared_ptr<tcp::socket>& socket) { 
        broadcast("todo: broadcast actual message");
        write(socket,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/plain\r\n"
          "Connection: close\r\n"
          "Cache-Control: no-cache\r\n"
          "\r\n",
          [this, socket](boost::system::error_code ec, std::size_t) {
            socket->shutdown(tcp::socket::shutdown_both, ec);
          });
      }}
    };
  }
  
  int _sse_client_count = 0;
  std::list<std::shared_ptr<tcp::socket>> _sse_clients;
  std::map<std::string, std::function<void(std::shared_ptr<tcp::socket>&)>> _handlers;
  tcp::acceptor _acceptor;
  tcp::socket _socket;
};

int main(int argc, char** argv) {
  short port = 1942; 
  
  // parse args
  std::vector<std::string> args(argv + 1, argv + argc);
  if (std::find(args.begin(), args.end(), "-h") != args.end()) {
    std::cout << argv[0] << " [-p port] [-i]" << std::endl;
    return 0;
  }
  bool run_interval = std::find(args.begin(), args.end(), "-i") != args.end();
  if (std::find(args.begin(), args.end(), "-p") != args.end()) {
    auto pos = std::find(args.begin(), args.end(), "-p");
    if (pos + 1 == args.end()) {
      std::cerr << "Invalid port argument" << std::endl;
      return 1;
    }
    port = std::stoi(*(pos + 1));
  }
  std::cout << "* Listening on port " << port << std::endl;
  boost::asio::io_service io_service;
  server s(io_service, port);
  if (run_interval) {
    // run an intervalled broadcast - don't bother cleaning the pointer
    std::cout << "* Broadcasting a very important message to all clients every second" << std::endl;
    interval* ival = new interval(io_service, 1, [&s]() {
      s.broadcast("I'm a teapot");
    });
    ival->start();
  }
  // start event loop
  io_service.run();
  return 0;
}
