#include "sse_client.hpp"
  
bool sse_client::is_dead() const {
  return _dead;
}

void sse_client::send(const std::string& msg) {
  if (_dead) return;
  auto buf = boost::asio::buffer(msg, msg.length());
  auto self(shared_from_this());
  boost::asio::async_write(*_socket, buf,
    [this, self](boost::system::error_code ec, std::size_t) {
      if (ec == boost::asio::error::eof ||
          ec == boost::asio::error::connection_reset ||
          ec == boost::asio::error::broken_pipe) {
        _dead = true;
      }
    });
}
