#pragma once

#include <memory>

#include "common.hpp"
#include "dispatcher.hpp"

namespace eagle {

class app;

class connection : public std::enable_shared_from_this<connection> {
 public:
  connection(base_dispatcher& dispt, tcp::socket socket)
      : dispatcher_(dispt), socket_(std::move(socket)) {}
  ~connection() = default;

 private:
  void handle_request() {
    http::async_read(socket_, buffer_, request_,
                     [conn = shared_from_this()](
                         beast::error_code ec, std::size_t bytes_transferred) {
                       // TODO: The dispatcher could fail, what do we do?
                       conn->dispatcher_.dispatch(conn->request_,
                                                  conn->response_);
                       conn->send_response();
                     });
  }

  void send_response() {
    response_.content_length(response_.body().size());
    http::async_write(
        socket_, response_,
        [conn = shared_from_this()](beast::error_code ec, std::size_t) {
          conn->socket_.shutdown(tcp::socket::shutdown_send, ec);
          conn->deadline_.cancel();
        });
  }

  void initiate_connection_deadline() {
    deadline_.async_wait([conn = shared_from_this()](beast::error_code ec) {
      conn->socket_.close(ec);
    });
  }

  tcp::socket& get_socket() { return socket_; }

 private:
  friend app;

  base_dispatcher& dispatcher_;

  tcp::socket socket_;
  beast::flat_buffer buffer_{8192};
  request request_;

  response response_;
  net::steady_timer deadline_{socket_.get_executor(), std::chrono::seconds(20)};
};
};  // namespace eagle
