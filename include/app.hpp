
#pragma once

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

#include "common.hpp"
#include "connection.hpp"
#include "dispatcher.hpp"
#include "handler.hpp"

namespace eagle {

DEFINE_string(address, "127.0.0.1", "Server address");
DEFINE_uint32(port, 3003, "Server port number");

class app {
 public:
  app(int argc, char* argv[]) : dispatcher_() {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();
  }

  ~app() = default;

  // TODO: Return a system error code here so that cleints can write:
  // int main() { return app.start(); }
  void start(std::optional<std::string> address = {},
             std::optional<uint16_t> port = {}) {
    server_address_ = address.value_or(FLAGS_address);
    server_port_ = port.value_or(FLAGS_port);

    tcp::acceptor acceptor{
        ioc_, {net::ip::make_address(server_address_), server_port_}};

    accept_connection(acceptor);
    LOG(INFO) << "Serving HTTP on " << server_address_ << " @ " << server_port_
              << " ..." << std::endl;
    ioc_.run();
  }

  void handle(http::verb method,
              std::string_view endpoint,
              handler_fn_type h_fn) {
    dispatcher_.add_handler(method, endpoint, h_fn);
  }

  void handle(std::string_view endpoint, stateful_handler& h_obj) {
    dispatcher_.add_handler(endpoint, h_obj);
  }

  template <typename Policy = intercept_policy_before>
  void intercept(interceptor_type inter) {
    dispatcher_.add_interceptor(Policy::value, inter);
  }

 private:
  void accept_connection(tcp::acceptor& acceptor) {
    acceptor.async_accept(socket_, [this, &acceptor](beast::error_code ec) {
      if (ec) {
        LOG(ERROR) << "Error: " << ec.message() << std::endl;
        exit(ec.value());
      }

      std::make_shared<connection>(dispatcher_, std::move(socket_))
          ->handle_request();

      accept_connection(acceptor);
    });
  }

 private:
  net::io_context ioc_{1};
  tcp::socket socket_{ioc_};
  dispatcher dispatcher_;
  std::string server_address_;
  uint16_t server_port_;
};
}  // namespace eagle