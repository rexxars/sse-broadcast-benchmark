#include <map>
#include <list>
#include <string>
#include <boost/asio.hpp>
#include <mutex>
#include "http_handler.hpp"
#include "sse_client.hpp"

using boost::asio::ip::tcp;

class sse_server {
public:
  sse_server(boost::asio::io_service& io_service, short port)
    : _acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
      _socket(io_service)
  {
    init_handlers();
    do_accept();
  }

  void broadcast(const std::string& msg);

private:
  typedef std::function<void(boost::system::error_code, std::size_t)> write_cb;

  void do_accept();
  void write(std::shared_ptr<tcp::socket>& socket, const std::string& msg, write_cb cb);
  void init_handlers();

  int _sse_client_count = 0;
  std::list<std::shared_ptr<sse_client>> _sse_clients;
  std::mutex _sse_clients_mutex;
  std::shared_ptr<http_handler::handler_map> _handlers;
  tcp::acceptor _acceptor;
  tcp::socket _socket;
};
