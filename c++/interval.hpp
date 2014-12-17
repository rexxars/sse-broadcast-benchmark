class interval {
public:
  interval(boost::asio::io_service& io, int seconds, std::function<void()> cb) 
    : _cb(cb),
      _timer(io, boost::posix_time::seconds(seconds)) {
    _seconds = seconds; 
  }

  void start() {
    _timer.async_wait([this](boost::system::error_code) { trigger(); });
  }

private:
  void trigger() {
    _cb();
    _timer.expires_from_now(boost::posix_time::seconds(_seconds));
    start();
  }

  int _seconds;
  std::function<void()> _cb;
  boost::asio::deadline_timer _timer;
};
