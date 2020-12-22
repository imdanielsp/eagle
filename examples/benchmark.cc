#include "eagle.hpp"

int main(int argc, const char** argv) {
  eagle::app app;

  app.handle(eagle::verb::get, "/test",
             [](const auto& req, auto& resp) -> bool {
               resp.json() << "{}";
               return true;
             });

  app.start();

  return 0;
}
