#include <map>
#include <atomic>
#include <list>
#include <string>
#include <boost/asio.hpp>
#include <mutex>
#include "http_handler.hpp"
#include "sse_client.hpp"
#include "singlylist.hpp"

using boost::asio::ip::tcp;

class sse_server {
public:
  sse_server(boost::asio::io_service& io_service, unsigned short port, unsigned short bucket_count, bool verbose)
    : _acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
      _socket(io_service),
      _sse_client_bucket_count(bucket_count),
      _verbose(verbose)
  {
    for (auto i = 0; i < bucket_count; ++i) {
      _sse_client_buckets.push_back(std::make_shared<singlylist<std::shared_ptr<sse_client>>>());
    }
    init_handlers();
    do_accept();
  }

  void broadcast(const std::string& msg);

private:
  typedef std::function<void(boost::system::error_code, std::size_t)> write_cb;
  typedef singlylist<std::shared_ptr<sse_client>> bucket_type;
  typedef std::shared_ptr<bucket_type> bucket_ptr_type;

  void do_accept();
  void write(std::shared_ptr<tcp::socket>& socket, const std::string& msg, write_cb cb);
  void init_handlers();

  bool _verbose;
  int _sse_client_count = 0;
  const int _sse_client_bucket_count;
  int _sse_client_bucket_counter = 0;
  std::vector<bucket_ptr_type> _sse_client_buckets;
  std::shared_ptr<http_handler::handler_map> _handlers;
  tcp::acceptor _acceptor;
  tcp::socket _socket;
};
