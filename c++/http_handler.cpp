#include <stdexcept>
#include <tuple>
#include "http_handler.hpp"

using boost::asio::ip::tcp;

class http_invalid_request : public std::exception {};

void http_handler::start() {
  _socket->set_option(tcp::no_delay(true));
  do_read();
}

void http_handler::do_read() {
  auto self(shared_from_this());
  _socket->async_read_some(boost::asio::buffer(&_buffer[_offset], max_length - _offset),
    [this, self](boost::system::error_code ec, std::size_t length) {
      if (!ec) {
        _offset += length;
        if (_buffer.find("\r\n\r\n") != std::string::npos) do_handle_request();
        else if (_offset >= max_length) return; // todo: flag disconnect, currently the connection will just die
        else do_read();
      }
    });
}

std::tuple<std::string, std::string> http_handler::get_method_path() {
  auto split_pos = _buffer.find(' ', 0);
  if (split_pos == std::string::npos) throw http_invalid_request();
  auto method = _buffer.substr(0, split_pos);
  split_pos += 1;
  if (_buffer.length() == split_pos) throw http_invalid_request();
  std::transform(method.begin(), method.end(), method.begin(), ::toupper);
  auto path_end = _buffer.find(' ', split_pos);
  if (path_end == std::string::npos) throw http_invalid_request();
  return std::make_tuple(method, _buffer.substr(split_pos, path_end - split_pos));
}

void http_handler::write(const std::string& msg, http_handler::write_cb cb) {
  boost::asio::async_write(*_socket, boost::asio::buffer(msg, msg.length()), cb);
}

void http_handler::do_handle_request() {
  std::tuple<std::string, std::string> method_path;
  try {
    method_path = get_method_path();
  }
  catch (http_invalid_request) {
    std::cerr << "Invalid path or method detected. Closing connection.\n";
    write(
      "HTTP/1.1 400 Bad request\r\n"
      "Connection: close\r\n"
      "\r\n",
      [this](boost::system::error_code, std::size_t) {
        boost::system::error_code ec;
        _socket->shutdown(tcp::socket::shutdown_both, ec);
      });
    return;
  }
  auto handler_key = std::get<0>(method_path) + " " + std::get<1>(method_path);
  std::function<void(std::shared_ptr<tcp::socket>&)> handler;
  try {
    handler = _handlers->at(handler_key);
  }
  catch (std::out_of_range) {
    write(
      "HTTP/1.1 404 Not found\r\n"
      "Connection: close\r\n"
      "\r\n",
      [this](boost::system::error_code, std::size_t) {
        boost::system::error_code ec;
        _socket->shutdown(tcp::socket::shutdown_both, ec);
      });
    return;
  }
  handler(_socket);
}

