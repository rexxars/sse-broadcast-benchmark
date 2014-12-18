#include <map>
#include <boost/asio.hpp>
#include <functional>
#include <tuple>

class http_handler
  : public std::enable_shared_from_this<http_handler> {
public:
  typedef std::map<std::string, std::function<void(std::shared_ptr<boost::asio::ip::tcp::socket>&)>> handler_map;
  
  http_handler(boost::asio::ip::tcp::socket socket, handler_map handlers)
    : _socket(std::move(socket)), 
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

  handler_map _handlers;
  boost::asio::ip::tcp::socket _socket;
  enum { max_length = 1024 };
  int  _offset = 0;
  std::string _buffer;
};
