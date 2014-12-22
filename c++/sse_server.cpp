#include "sse_server.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <functional>

void sse_server::broadcast(const std::string& msg) {
  std::string full_msg = "data: " + msg + "\n\n";
  // broadcast to all
  for (auto& bucket : _sse_client_buckets) {
    bucket->mutex.lock();
    auto i = std::begin(bucket->clients);
    while (i != std::end(bucket->clients)) {
      if ((*i)->is_dead()) {
        if (i == std::end(bucket->clients)) continue;
        auto tmp = i;
        ++i;
        bucket->clients.erase(tmp);
        --_sse_client_count;
      }
      else {
        (*i)->send(full_msg);
        ++i;
      }
    }
    bucket->mutex.unlock();
  }
}

void sse_server::do_accept() {
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

void sse_server::write(std::shared_ptr<tcp::socket>& socket, const std::string& msg, write_cb cb) {
  boost::asio::async_write(*socket, boost::asio::buffer(msg, msg.length()), cb);
}

void sse_server::init_handlers() {
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
            auto& bucket = _sse_client_buckets[_bucket_roundrobin_counter % _sse_client_bucket_count];
            ++_bucket_roundrobin_counter;
            bucket->mutex.lock();
            bucket->clients.push_back(std::make_shared<sse_client>(std::move(socket)));
            bucket->mutex.unlock();
            ++_sse_client_count;
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
