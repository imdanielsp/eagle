#ifndef EAGLE_CONNECTION_HPP
#define EAGLE_CONNECTION_HPP

#include <memory>

#include "common.hpp"
#include "dispatcher.hpp"

namespace eagle {

class connection_interface {
 public:
  connection_interface() = default;
  virtual ~connection_interface() = default;

  virtual void handle_data() = 0;
  virtual void send_data() = 0;
};

class connection final : public connection_interface,
                         public std::enable_shared_from_this<connection> {
 public:
  connection(dispatcher_interface& dispt, tcp::socket socket)
      : dispatcher_(dispt), socket_(std::move(socket)) {}
  ~connection() = default;

  void handle_data() override { handle_request_(); }

  void send_data() override { send_response_(); }

 private:
  void handle_request_() {
    http::async_read(
        socket_, buffer_, request_.raw(),
        [conn = shared_from_this()](beast::error_code ec,
                                    std::size_t bytes_transferred) {
          auto peer_addrs =
              conn->socket_.remote_endpoint().address().to_string();
          conn->request_.peer(peer_addrs);
          // TODO: The dispatcher could fail, what do we do?
          conn->dispatcher_.dispatch(conn->request_, conn->response_);
          conn->send_data();
        });
  }

  void send_response_() {
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

 private:
  dispatcher_interface& dispatcher_;

  tcp::socket socket_;
  beast::flat_buffer buffer_{8192};
  request request_;

  response response_;
  net::steady_timer deadline_{socket_.get_executor(), std::chrono::seconds(10)};
};
};  // namespace eagle

#endif  // EAGLE_CONNECTION_HPP
