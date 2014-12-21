#include "sse_client.hpp"
  
bool sse_client::is_dead() const {
  return _dead;
}

void sse_client::send(const std::string& msg) {
  if (_dead) return;
  std::string full_msg = "data: " + msg + "\n\n";
  auto buf = boost::asio::buffer(full_msg, full_msg.length());
  auto self(shared_from_this());
  boost::asio::async_write(*_socket, buf,
    [this, self](boost::system::error_code ec, std::size_t) {
      // clients are lazily removed, meaning we won't bother
      // detecting disconnects until a failed write is flagged
      if (ec == boost::asio::error::eof ||
          ec == boost::asio::error::connection_reset ||
          ec == boost::asio::error::broken_pipe) {
        _dead = true;
      }
    });
}
