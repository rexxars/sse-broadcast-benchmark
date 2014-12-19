#include <stdexcept>
#include <tuple>
#include "http_handler.hpp"

using boost::asio::ip::tcp;
using boost::asio::buffer;

class http_invalid_request : public std::exception {};

void http_handler::start() {
  _socket->set_option(tcp::no_delay(true));
  do_read_header();
}

void http_handler::do_read_header() {
  auto self(shared_from_this());
  auto chunk = std::make_shared<std::vector<char>>(hdr_chunk_size);
  _socket->async_read_some(buffer(*chunk),
    [this, chunk, self](boost::system::error_code ec, std::size_t length) {
      if (!ec) {
        _buffer.append(chunk->begin(), chunk->begin() + length);
        _buffered += length;
        if (_buffer.find("\r\n\r\n") != std::string::npos) do_handle_request();
        else if (_buffered >= hdr_max_size) return; // todo: flag disconnect, currently the connection will just die
        else do_read_header();
      }
    });
}

void http_handler::do_read_body(int remaining_size, std::function<void()> cb) {
  auto self(shared_from_this());
  auto chunk = std::make_shared<std::vector<char>>(
    remaining_size < body_chunk_size ? remaining_size : body_chunk_size
  );
  _socket->async_read_some(buffer(*chunk),
    [this, self, chunk, remaining_size, cb](boost::system::error_code ec, std::size_t length) {
      if (!ec) {
        _buffer.append(chunk->begin(), chunk->begin() + length);
        int new_remaining_size = remaining_size - length;
        if (new_remaining_size <= 0) cb();
        else do_read_body(new_remaining_size, cb);
      }
    });
}

http_handler::request_info http_handler::parse_request() {
  auto split_pos = _buffer.find(' ', 0);
  if (split_pos == std::string::npos) throw http_invalid_request();
  auto method = _buffer.substr(0, split_pos);
  split_pos += 1;
  if (_buffer.length() == split_pos) throw http_invalid_request();
  std::transform(method.begin(), method.end(), method.begin(), ::toupper);
  auto path_end = _buffer.find(' ', split_pos);
  if (path_end == std::string::npos) throw http_invalid_request();
  auto path = _buffer.substr(split_pos, path_end - split_pos);

  header_map headers;
  auto iterator = _buffer.begin() + path_end + 1;
  // skip beyond " HTTP/1.x\r\n"
  while (iterator != _buffer.end()) {
    if (*iterator == '\n') break;
    ++iterator;
  }
  ++iterator;
  // list all remaining headers
  auto capture_start = iterator;
  while (iterator != _buffer.end()) {
    if (*iterator == ':') {
      std::string header = _buffer.substr(capture_start - _buffer.begin(), iterator - capture_start);
      std::transform(header.begin(), header.end(), header.begin(), ::tolower);
      ++iterator; // move beyond ':'
      while (true) {
        if (iterator == _buffer.end()) throw http_invalid_request(); // missing header value
        else if (*iterator != ' ') break; // header value found
        ++iterator;
      }
      capture_start = iterator;
      while (true) {
        if (iterator == _buffer.end()) throw http_invalid_request(); // missing header end
        else if (*iterator == '\n') {
          std::string value = _buffer.substr(capture_start - _buffer.begin(), iterator - capture_start);
          headers[header] = value;
          break;
        }
        ++iterator;
      }
      ++iterator;
      capture_start = iterator;
    }
    else if (*iterator == '\n') {
      // look for second newline, marking the end of the header block
      while (true) {
        if (iterator == _buffer.end()) throw http_invalid_request(); // missing header value
        else if (*iterator == '\n') {
          ++iterator; // move past newline
          break;
        }
        else if (*iterator != '\r') throw http_invalid_request(); // invalid symbol: not CR or LF
        ++iterator;
      }
      _body_offset = iterator - _buffer.begin(); 
      break; // end of header should be hit
    }
    else ++iterator;
  }
  _has_body = headers.find("content-length") != headers.end();
  
  return std::make_tuple(method, path, headers);
}

void http_handler::write(const std::string& msg, http_handler::write_cb cb) {
  boost::asio::async_write(*_socket, boost::asio::buffer(msg, msg.length()), cb);
}

void http_handler::do_handle_request() {
  auto self(shared_from_this());
  request_info req;
  try {
    req = parse_request();
  }
  catch (http_invalid_request) {
    //std::cerr << "Invalid path or method detected. Closing connection.\n";
    write(
      "HTTP/1.1 400 Bad request\r\n"
      "Connection: close\r\n"
      "\r\n",
      [this, self](boost::system::error_code ec, std::size_t) {
        if (!ec) _socket->shutdown(tcp::socket::shutdown_both, ec);
      });
    return;
  }
  auto handler_key = std::get<0>(req) + " " + std::get<1>(req);
  handler_func handler;
  try {
    handler = _handlers->at(handler_key);
  }
  catch (std::out_of_range ex) {
    write(
      "HTTP/1.1 404 Not found\r\n"
      "Connection: close\r\n"
      "\r\n",
      [this, self](boost::system::error_code ec, std::size_t) {
        if (!ec) _socket->shutdown(tcp::socket::shutdown_both, ec);
      });
    return;
  }

  auto headers = std::get<2>(req);
  std::size_t body_size = 0;
  if (_has_body) {
    body_size = std::stoi(headers["content-length"]);
    if (body_size > body_max_size) {
      write(
        "HTTP/1.1 400 Not found\r\n"
        "Connection: close\r\n"
        "\r\n",
        [this, self](boost::system::error_code ec, std::size_t) {
          if (!ec) _socket->shutdown(tcp::socket::shutdown_both, ec);
        });
      return;
    }
  }

  handler(_socket, [this, self, body_size](std::function<void(std::string)> cb) {
    if (!_has_body) {
      cb("");
      return;
    }
    int buffered_body_size = _buffered - _body_offset;
    int remaining_size = body_size - buffered_body_size;
    if (remaining_size <= 0) cb(_buffer.substr(_body_offset));
    else {
      do_read_body(remaining_size,
        [this, self, cb]() {
          cb(_buffer.substr(_body_offset));
        });
    }
  });
}

