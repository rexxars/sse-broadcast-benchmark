#include <iostream>
#include <functional>
#include <map>
#include <list>
#include <string>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <thread>
#include <mutex>
#include "interval.hpp"
#include "http_handler.hpp"
#include "sse_handler.hpp"

using boost::asio::ip::tcp;

class server
{
public:
  server(boost::asio::io_service& io_service, short port)
    : _acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
      _socket(io_service)
  {
    init_handlers();
    do_accept();
  }

  void broadcast(const std::string& msg) {
    std::string full_msg = "data: " + msg + "\n\n";
    auto buf = boost::asio::buffer(full_msg, full_msg.length());
    auto i = std::begin(_sse_clients);
    _sse_clients_mutex.lock();
    while (i != std::end(_sse_clients)) {
      boost::asio::async_write(**i, buf,
        [this, i](boost::system::error_code ec, std::size_t) {
          // clients are lazily removed, meaning we won't bother
          // detecting disconnects until a failed write is flagged
          if (ec == boost::asio::error::eof ||
              ec == boost::asio::error::connection_reset ||
              ec == boost::asio::error::broken_pipe) {
            _sse_clients_mutex.lock();
            _sse_clients.erase(i);
            --_sse_client_count;
            _sse_clients_mutex.unlock();
          }
        });
      ++i;
    }
    _sse_clients_mutex.unlock();
  }

private:
  typedef std::function<void(boost::system::error_code, std::size_t)> write_cb;

  // accept loop
  void do_accept() {
    _acceptor.async_accept(_socket,
        [this](boost::system::error_code ec) {
          if (!ec) {
            // pass control to the http handler
            auto socketptr = std::make_shared<tcp::socket>(std::move(_socket));
            std::make_shared<http_handler>(socketptr, _handlers)->start();
          }
          do_accept();
        });
  }

  void write(std::shared_ptr<tcp::socket>& socket, const std::string& msg, write_cb cb) {
    boost::asio::async_write(*socket, boost::asio::buffer(msg, msg.length()), cb);
  }

  // minimal http request handlers
  void init_handlers() {
    http_handler::handler_map handlers = {
      {"GET /connections", [this](std::shared_ptr<tcp::socket>& socket, http_handler::read_body_func read_body) {
        std::string msg = boost::str(boost::format("%d") % _sse_client_count);
        write(socket,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: " + boost::str(boost::format("%d") % msg.length()) + "\r\n"
          "Connection: close\r\n"
          "Access-Control-Allow-Origin: *\r\n"
          "Cache-Control: no-cache\r\n"
          "\r\n" + msg,
          [this, socket](boost::system::error_code, std::size_t) {
            boost::system::error_code ec;
            socket->shutdown(tcp::socket::shutdown_both, ec);
          });
      }},
      {"GET /sse", [this](std::shared_ptr<tcp::socket>& socket, http_handler::read_body_func read_body) {
        write(socket,
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/event-stream\r\n"
          "Connection: keep-alive\r\n"
          "Access-Control-Allow-Origin: *\r\n"
          "Cache-Control: no-cache\r\n"
          "\r\n"
          ":ok\n\n",
          [this, socket](boost::system::error_code ec, std::size_t) {
            if (ec) {
              socket->shutdown(tcp::socket::shutdown_both, ec);
            }
            else {
              _sse_clients_mutex.lock();
              _sse_clients.push_back(std::move(socket));
              ++_sse_client_count;
              _sse_clients_mutex.unlock();
            }
          });
      }},
      {"OPTIONS /connections", [this](std::shared_ptr<tcp::socket>& socket, http_handler::read_body_func read_body) {
        write(socket,
          "HTTP/1.1 204 No Content\r\n"
          "Connection: close\r\n"
          "Access-Control-Allow-Origin: *\r\n"
          "\r\n",
          [this, socket](boost::system::error_code, std::size_t) {
            boost::system::error_code ec;
            socket->shutdown(tcp::socket::shutdown_both, ec);
          });
      }},
      {"OPTIONS /sse", [this](std::shared_ptr<tcp::socket>& socket, http_handler::read_body_func read_body) {
        write(socket,
          "HTTP/1.1 204 No Content\r\n"
          "Connection: close\r\n"
          "Access-Control-Allow-Origin: *\r\n"
          "\r\n",
          [this, socket](boost::system::error_code, std::size_t) {
            boost::system::error_code ec;
            socket->shutdown(tcp::socket::shutdown_both, ec);
          });
      }},
      {"POST /broadcast", [this](std::shared_ptr<tcp::socket>& socket, http_handler::read_body_func read_body) {
        read_body(
          [this, &socket](std::string body) {
            broadcast(body);
            write(socket,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
              "Connection: close\r\n"
              "Cache-Control: no-cache\r\n"
              "\r\n",
              [this, socket](boost::system::error_code ec, std::size_t) {
                socket->shutdown(tcp::socket::shutdown_both, ec);
              });
          });
      }}
    };
    _handlers = std::make_shared<http_handler::handler_map>(std::move(handlers));
  }

  int _sse_client_count = 0;
  std::list<std::shared_ptr<tcp::socket>> _sse_clients;
  std::mutex _sse_clients_mutex;
  std::shared_ptr<http_handler::handler_map> _handlers;
  tcp::acceptor _acceptor;
  tcp::socket _socket;
};

int main(int argc, char** argv) {
  unsigned short thread_count = 1;
  unsigned short port = 1942;

  // parse args
  std::vector<std::string> args(argv + 1, argv + argc);
  if (std::find(args.begin(), args.end(), "-h") != args.end()) {
    std::cout << argv[0] << " [-p port] [-t threads] [--time-broadcast]" << std::endl;
    return 0;
  }

  bool enable_interval = std::find(args.begin(), args.end(), "--time-broadcast") != args.end();

  if (std::find(args.begin(), args.end(), "-p") != args.end()) {
    auto pos = std::find(args.begin(), args.end(), "-p");
    if (pos + 1 == args.end()) {
      std::cerr << "Invalid port argument" << std::endl;
      return 1;
    }
    port = std::stoi(*(pos + 1));
  }
  std::cout << "* Listening on port " << port << std::endl;

  if (std::find(args.begin(), args.end(), "-t") != args.end()) {
    auto pos = std::find(args.begin(), args.end(), "-t");
    if (pos + 1 == args.end()) {
      std::cerr << "Invalid thread argument" << std::endl;
      return 1;
    }
    thread_count = std::stoi(*(pos + 1));
  }

  // init server
  boost::asio::io_service io_service;
  server s(io_service, port);
  if (enable_interval) {
    // run an intervalled broadcast - don't bother cleaning the pointer
    interval* ival = new interval(io_service, 1, [&s]() {
      struct timeval tp;
      gettimeofday(&tp, NULL);
      s.broadcast(std::to_string(tp.tv_sec * 1000 + tp.tv_usec / 1000));
    });
    ival->start();
  }

  // start event loop
  std::vector<std::thread> threads;
  for (int i = 0; i < thread_count; ++i) {
    threads.push_back(std::thread(
      [&io_service]() {
        io_service.run();
      }));
  }
  std::cout << "* Started " << thread_count << " threads" << std::endl;
  for(auto& thread : threads) {
    thread.join();
  }

  return 0;
}
