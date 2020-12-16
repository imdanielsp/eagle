#ifndef EAGLE_COMMON_HPP
#define EAGLE_COMMON_HPP

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>

#include "request.hpp"

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

// TODO: Remove when glog works...
#undef LOG
#define LOG(serv) std::cout

namespace eagle {
using response = http::response<http::dynamic_body>;

using interceptor_type = std::function<void(const request&, response&)>;

enum class interception_policy { after = 0, before = 1 };

struct intercept_policy_after {
  static const interception_policy value = interception_policy::after;
};

struct intercept_policy_before {
  static const interception_policy value = interception_policy::before;
};

}  // namespace eagle

#endif  // EAGLE_COMMON_HPP
