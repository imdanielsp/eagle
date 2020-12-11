
#pragma once

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

#include "common.hpp"
#include "connection.hpp"
#include "dispatcher.hpp"
#include "handler.hpp"

namespace eagle {

DEFINE_string(address, "127.0.0.1", "Server address");
DEFINE_uint32(port, 3003, "Server port number");

// Forward declaration of the `app` template class.
template <typename ConnectionType = connection>
class app;

// Template deduction guide for the initialization. This tells the compiler,
// that when you call an app constructor matching `app(int argc, char* argv[])`,
// the type of the template should be `app<ConnectionType>`, which by default is
// an `eagle::connection`. This deduction allows clien code to instanciate
// `eagle::app` objects without passing any templates parameters and not using
// the empty template syntax: `eagle::app<> app(...)`. In addition, we get to
// use dependecy injection via a template parameter for the connection given
// that `ConnectionType` implements `connection_interface`. Unit test can then
// write `eagle::app<mock_connection> app(...)` and test the app without an
// actual TCP connection.
//
/// See:
// https://isocpp.org/blog/2017/09/quick-q-what-are-template-deduction-guides-in-cpp17
template <typename ConnectionType = connection>
app(int argc, char* argv[]) -> app<ConnectionType>;

template <typename ConnectionType>
class app {
  static_assert(std::is_base_of<connection_interface, ConnectionType>::value,
                "ConnectionType must implement connection_interface");
  static_assert(std::is_final<ConnectionType>::value,
                "ConnectionType should be final");

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

  void handle(std::string_view endpoint, handler_type& h_obj) {
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

      std::make_shared<ConnectionType>(dispatcher_, std::move(socket_))
          ->handle_data();

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