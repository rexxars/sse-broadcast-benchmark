#include "sse_server.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <functional>
#include <forward_list>

void sse_server::broadcast(const std::string& msg) {
  std::string full_msg = "data: " + msg + "\n\n";
  std::forward_list<std::tuple<bucket_ptr_type&, bucket_type::NODE_PTR&>> dead_nodes;
  for (auto& bucket : _sse_client_buckets) {
    bucket->lock();
    auto iterator = bucket->get_front();
    while (iterator != nullptr) {
      if (iterator->data->is_dead()) dead_nodes.push_front(std::make_tuple(std::ref(bucket), std::ref(iterator)));
      else iterator->data->send(full_msg);
      iterator = iterator->next;
    }
    bucket->unlock();
  }
  for (auto& dead_node : dead_nodes) {
    auto& bucket = std::get<0>(dead_node);
    bucket->lock();
    auto& node = std::get<1>(dead_node);
    bucket->remove(node);
    --_sse_client_count;
    bucket->unlock();
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
            auto& bucket = _sse_client_buckets[_sse_client_bucket_counter % _sse_client_bucket_count];
            ++_sse_client_bucket_counter;
            bucket->push(std::make_shared<sse_client>(std::move(socket)));
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
