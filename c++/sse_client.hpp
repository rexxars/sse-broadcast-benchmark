#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class sse_client 
  : public std::enable_shared_from_this<sse_client> {
public:
  sse_client(std::shared_ptr<tcp::socket> socket)
    : _socket(socket) {
  }
  
  bool is_dead() const;
  void send(const std::string& msg);

private:
  std::shared_ptr<tcp::socket> _socket;
  bool _dead = false;
};
