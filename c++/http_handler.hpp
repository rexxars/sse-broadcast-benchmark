#include <map>
#include <boost/asio.hpp>
#include <functional>
#include <tuple>

class http_handler
  : public std::enable_shared_from_this<http_handler> {
public:
  typedef std::map<std::string, std::function<void(std::shared_ptr<boost::asio::ip::tcp::socket>&)>> handler_map;
  
  http_handler(std::shared_ptr<boost::asio::ip::tcp::socket>& socket, std::shared_ptr<handler_map> handlers)
    : _socket(socket), 
      _handlers(handlers), 
      _buffer(max_length, 0) {
  }

  void start();

private:
  typedef std::function<void(boost::system::error_code, std::size_t)> write_cb;

  void do_read();
  void do_handle_request();
  void write(const std::string& msg, write_cb cb);
  std::tuple<std::string, std::string> get_method_path();

  std::shared_ptr<handler_map> _handlers;
  std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
  enum { max_length = 512 };
  std::size_t  _offset = 0;
  std::string _buffer;
};
