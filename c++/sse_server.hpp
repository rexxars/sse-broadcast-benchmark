#include <map>
#include <list>
#include <string>
#include <boost/asio.hpp>
#include <mutex>
#include "http_handler.hpp"
#include "sse_client.hpp"

using boost::asio::ip::tcp;

struct sse_client_bucket {
  std::mutex mutex;
  std::list<std::shared_ptr<sse_client>> clients;
};

class sse_server {
public:
  sse_server(boost::asio::io_service& io_service, short port)
    : _acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
      _socket(io_service)
  {
    for (int i = 0; i < _sse_client_bucket_count; ++i) {
      _sse_client_buckets.push_back(std::make_shared<sse_client_bucket>());
    }
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
  const int _sse_client_bucket_count = 100;
  std::vector<std::shared_ptr<sse_client_bucket>> _sse_client_buckets;
  std::shared_ptr<http_handler::handler_map> _handlers;
  tcp::acceptor _acceptor;
  tcp::socket _socket;
};
