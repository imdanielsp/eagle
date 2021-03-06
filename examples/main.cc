#include <eagle.hpp>
#include <iostream>

class h : public eagle::stateful_handler_base {
 public:
  h() {}
  virtual ~h() {}

  bool get(const eagle::request&, eagle::response& resp) override {
    resp.html() << "<p>Dispatched by the handler object! Count "
                << state_.count_++ << "</p>";
    return true;
  }

 private:
  struct application_state {
    size_t count_{0};
  } state_;
};

int main(int argc, char* argv[]) {
  eagle::app app;

  app.intercept([](const auto&, auto& resp) {
    std::cout << "Intercepting before the handler is called" << std::endl;
  });

  app.intercept<eagle::intercept_policy_after>([](const auto&, auto& resp) {
    std::cout << "Intercepting after the handler is called" << std::endl;
  });

  app.handle(http::verb::get, "/hello-eagle",
             [](const auto& req, auto& resp) -> bool {
               resp.html() << "<h1>Hello Eagle! &#x1F985</h1>";
               return true;
             });

  app.handle(http::verb::post, "/hello-eagle", [](const auto& req, auto& resp) {
    resp.html() << "<h1>Some error happen</h1>";
    return false;
  });

  app.handle(http::verb::get, "/json", [](const auto& req, auto& resp) {
    resp.json() << "{ \"id\": 1234 }";
    return true;
  });

  h handler;
  app.handle("/api/v1/users", handler);

  app.handle(http::verb::get, "/user/{integer:id}",
             [](const auto& req, auto& resp) -> bool {
               auto userId = req.args().template get<int>("id");
               resp.html() << "<h3>User id: " << userId << "</h1>";
               return true;
             });

  app.handle(http::verb::get, "/user/{integer:id}/type/{string:t}",
             [](const auto& req, auto& resp) -> bool {
               auto userId = req.args().template get<int>("id");
               auto type = req.args().template get<std::string_view>("t");
               resp.html() << "<h3>User id: " << userId << " type: " << type
                           << "</h3>";
               return true;
             });

  app.start();

  return 0;
}
