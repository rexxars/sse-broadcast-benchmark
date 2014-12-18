#include <boost/asio.hpp>

class sse_handler
  : public std::enable_shared_from_this<sse_handler> {
public:
  sse_handler(boost::asio::ip::tcp::socket socket)
    : _socket(std::move(socket)) {
  }
  void start();
  void send(const std::string& message);

private:
  boost::asio::ip::tcp::socket _socket;
};
