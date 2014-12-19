#include <map>
#include <boost/asio.hpp>
#include <functional>
#include <tuple>

class http_handler
  : public std::enable_shared_from_this<http_handler> {
public:
  typedef std::function<void(std::function<void(std::string)>)> read_body_func;
  typedef std::function<void(std::shared_ptr<boost::asio::ip::tcp::socket>&, read_body_func)> handler_func;
  typedef std::map<std::string, handler_func> handler_map;
  
  http_handler(std::shared_ptr<boost::asio::ip::tcp::socket>& socket, std::shared_ptr<handler_map>& handlers)
    : _socket(socket), 
      _handlers(handlers) { }

  void start();

private:
  typedef std::function<void(boost::system::error_code, std::size_t)> write_cb;
  typedef std::map<std::string, std::string> header_map;
  typedef std::tuple<std::string, std::string, header_map> request_info;

  void do_read_header();
  void do_read_body(int remaining_size, std::function<void()> cb);
  void do_handle_request();
  void write(const std::string& msg, write_cb cb);
  request_info parse_request();

  std::shared_ptr<handler_map> _handlers;
  std::shared_ptr<boost::asio::ip::tcp::socket> _socket;
  std::size_t _body_offset;
  bool _has_body;
  enum { 
    hdr_chunk_size = 256,
    hdr_max_size = 2048,
    body_chunk_size = 64, // optimize for small bodies
    body_max_size = 65536
  };
  std::size_t _buffered = 0;
  std::string _buffer;
};
