using boost::asio::ip::tcp;

class sse_client 
  : public std::enable_shared_from_this<sse_client> {
public:
  sse_client(std::shared_ptr<tcp::socket> socket)
    : _socket(socket) {
  }

  void kill() {
    _dead = true;
  }

  bool is_dead() const {
    return _dead;
  }

  void send(const std::string& msg) {
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
          kill();
        }
      });
  }

private:
  std::shared_ptr<tcp::socket> _socket;
  bool _dead = false;
};
