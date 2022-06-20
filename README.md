![Eagle CI Build](https://github.com/imdanielsp/eagle/workflows/Eagle%20CI%20Build/badge.svg)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fimdanielsp%2Feagle.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fimdanielsp%2Feagle?ref=badge_shield)

# Eagle
A Minimalistic C++ Web Framework build on top of Boost Beast and ASIO

## Example

The `eagle::app` object is the main interface for installing handlers and interceptors (more on that later):

```c++
#include "eagle.hpp"

int main(argc, argv) {
  eagle::app app{argc, argv};
  
  app.handle(http::verb::get, "/hello-eagle",
           [](const auto& req, auto& resp) -> bool {
             resp.html() << "<h1>Eagle!</h1>";
             return true;
           });

  app.start();

  return 0;
}
```

Eagle also support object handlers that can encapsulate application's state:

```c++

#include "eagle.hpp"


class h : public eagle::handler_object {
 public:
  h() {}
  virtual ~h() {}

  bool get(const eagle::request&, eagle::response& resp) override {
    resp.html()
        << "<p>Dispatched by the handler object! Count " << state_.count_++ << "</p>";
    return true;
  }

 private:
  struct application_state {
    size_t count_{0};
  } state_;
};

int main(argc, argv) {

  eagle::app app{argc, argv};

  h handler;
  app.handle("/api/v1/users", handler);

  app.start();

  return 0;
}
```

## Building
Eagle uses `meson` as the build system and depends on the Boost.Beast library

```bash
$ brew install meson
$ brew install boost
$ meson builddir && cd builddir
$ meson compile
$ ./eagle
```

## Supported Method
Eagle's design principles are focus towards REST, the following methods are inherently supported in the interfaces:
- GET
- POST
- PUT
- DELETE


# Roadmap
- Hide the `boost::beast` dependecy
- Implement a message modifier interface rather than intereact with a `http::resp` directly
- Resource Id mapping
- Query parameters
- TBD


## License
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fimdanielsp%2Feagle.svg?type=large)](https://app.fossa.com/projects/git%2Bgithub.com%2Fimdanielsp%2Feagle?ref=badge_large)