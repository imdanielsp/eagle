#ifndef EAGLE_REQUEST_HPP
#define EAGLE_REQUEST_HPP

#include <boost/beast.hpp>
#include "params.hpp"

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace http = beast::http;    // from <boost/beast/http.hpp>

namespace eagle {

using verb = http::verb;

/// This class encapsulates a eagle request based on the Boost.Beast request
/// class implementation. The goal is to hide the implementation detail from
/// the handler and do heavy lifting work for the user in this object. For
/// example, parsing URI resource ids, JSON and more that Boost.Best doesn't
/// provide.
class request final {
 public:
  request() = default;

  request(http::request<http::dynamic_body>&& req) : request_(std::move(req)) {}

  ~request() = default;

  std::string_view target() const {
    auto target_sv = request_.target();
    return std::string_view{target_sv.data(), target_sv.size()};
  }

  void target(std::string_view&& sv) {
    request_.target(beast::string_view{sv.data(), sv.size()});
  }

  verb method() const { return request_.method(); }

  void method(verb v) { request_.method(v); }

  std::string_view peer() const { return peer_; }

  void peer(std::string_view p) { peer_ = p; }

  unsigned int version() const { return request_.version(); }

  auto& raw() { return request_; }

  const auto& params() const { return params_; }

  auto& params_ref() { return params_; }

 private:
  class params params_;
  http::request<http::dynamic_body> request_;
  std::string_view peer_;
};

}  // namespace eagle

#endif  // EAGLE_REQUEST_HPP
