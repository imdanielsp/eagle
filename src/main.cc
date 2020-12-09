#include <iostream>
#include "eagle.hpp"

class h : public eagle::stateful_handler {
 public:
  h() {}
  virtual ~h() {}

  bool get(const eagle::request&, eagle::response& resp) override {
    resp.set(http::field::content_type, "text/html");
    boost::beast::ostream(resp.body())
        << "Dispatched by the handler object! Count " << state_.count_++;
    return true;
  }

 private:
  struct application_state {
    size_t count_{0};
  } state_;
};

int main(int argc, char* argv[]) {
  eagle::app app{argc, argv};

  app.intercept([](const auto&, auto& resp) {
    std::cout << "Intercepting before the handler is called" << std::endl;
  });

  app.intercept<eagle::intercept_policy_after>([](const auto&, auto& resp) {
    std::cout << "Intercepting after the handler is called" << std::endl;
  });

  app.handle(http::verb::get, "/hello-eagle",
             [](const auto& req, auto& resp) -> bool {
               resp.set(http::field::content_type, "text/html");
               beast::ostream(resp.body()) << "<h1>Eagle!</h1>";
               return true;
             });

  app.handle(http::verb::post, "/hello-eagle", [](const auto& req, auto& resp) {
    resp.set(http::field::content_type, "text/html");
    beast::ostream(resp.body()) << "<h1>Some error happen</h1>";
    return false;
  });

  app.handle(http::verb::get, "/json", [](const auto& req, auto& resp) {
    resp.set(http::field::content_type, "application/json");
    beast::ostream(resp.body()) << "{ \"id\": 1234 }";
    return true;
  });

  h handler;
  app.handle("/api/v1/users", handler);

  app.start();

  return 0;
}
