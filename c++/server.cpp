#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include "interval.hpp"
#include "sse_server.hpp"

int main(int argc, char** argv) {
  unsigned short thread_count = 10;
  unsigned short port = 1942;

  // parse args
  std::vector<std::string> args(argv + 1, argv + argc);
  if (std::find(args.begin(), args.end(), "-h") != args.end()) {
    std::cout << argv[0] << " [-p port] [-t threads] [--time-broadcast]" << std::endl;
    return 0;
  }

  bool enable_interval = std::find(args.begin(), args.end(), "--time-broadcast") != args.end();

  if (std::find(args.begin(), args.end(), "-p") != args.end()) {
    auto pos = std::find(args.begin(), args.end(), "-p");
    if (pos + 1 == args.end()) {
      std::cerr << "Invalid port argument" << std::endl;
      return 1;
    }
    port = std::stoi(*(pos + 1));
  }
  std::cout << "* Listening on port " << port << std::endl;

  if (std::find(args.begin(), args.end(), "-t") != args.end()) {
    auto pos = std::find(args.begin(), args.end(), "-t");
    if (pos + 1 == args.end()) {
      std::cerr << "Invalid thread argument" << std::endl;
      return 1;
    }
    thread_count = std::stoi(*(pos + 1));
  }

  // init server
  boost::asio::io_service io_service;
  sse_server s(io_service, port);
  if (enable_interval) {
    // run an intervalled broadcast - don't bother cleaning the pointer
    interval* ival = new interval(io_service, 1, [&s]() {
      struct timeval tp;
      gettimeofday(&tp, NULL);
      s.broadcast(std::to_string(tp.tv_sec * 1000 + tp.tv_usec / 1000));
    });
    ival->start();
  }

  // start event loop
  std::vector<std::thread> threads;
  for (int i = 0; i < thread_count; ++i) {
    threads.push_back(std::thread(
      [&io_service]() {
        io_service.run();
      }));
  }
  std::cout << "* Started " << thread_count << " threads" << std::endl;
  for(auto& thread : threads) {
    thread.join();
  }

  return 0;
}
