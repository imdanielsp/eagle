#ifndef EAGLE_DISPATCHER_HPP
#define EAGLE_DISPATCHER_HPP

#include <array>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <chrono>
#include <forward_list>
#include <functional>
#include <optional>
#include <string_view>
#include <unordered_map>

#include "common.hpp"
#include "handler.hpp"
#include "handler_registry.hpp"

using namespace boost::adaptors;
using namespace boost::range;

namespace eagle {

class dispatcher_interface {
 public:
  dispatcher_interface() {}
  virtual ~dispatcher_interface() = default;

  virtual bool add_handler(http::verb method,
                           std::string_view endpoint,
                           handler_fn_type h_fn) = 0;

  virtual bool add_handler(std::string_view endpoint, handler_type& h_obj) = 0;

  virtual bool dispatch(request& request, response& response) = 0;
};

class dispatcher : public dispatcher_interface {
 public:
  dispatcher() {}
  ~dispatcher() = default;

  void add_interceptor(interception_policy policy, interceptor_type inter) {
    interceptors_.push_front({policy, inter});
  }

  bool add_handler(http::verb method,
                   std::string_view endpoint,
                   handler_fn_type h_fn) override {
    return install_fn_handler_(method, endpoint, h_fn);
  }

  bool add_handler(std::string_view endpoint, handler_type& h_obj) override {
    return install_object_handler_(endpoint, h_obj);
  }

  bool dispatch(request& req, response& resp) override {
    execute_interceptors_with_(intercept_policy_before::value, req, resp);

    bool status = true;
    auto target_endpoint =
        std::string_view(req.target().data(), req.target().size());

    if (has_object_handler_for_(target_endpoint)) {
      auto [opt_handler, _] = handler_object_registry_.get_handler_for(
          all_method, target_endpoint, req.args());
      status = dispatch_with_(opt_handler.value().get(), req, resp);
    } else if (has_function_handler_for_endpoint_and_method_(target_endpoint,
                                                             req.method())) {
      auto [opt_handler, _] = handler_fn_registry_.get_handler_for(
          req.method(), target_endpoint, req.args());
      status = dispatch_with_(opt_handler.value(), req, resp);
    } else if (has_at_least_one_function_handler_for_(target_endpoint)) {
      // If we are here it means that there isn't a object handler nor a
      // function handler for the (method, endpoint) pair, but there at least
      // one handler installed for the endpoint; therefore we dispatch a method
      // not allow error.
      status = dispatch_method_not_allow_(resp);
    } else {
      status = dispatch_not_found_(resp);
    }

    if (!status) {
      resp.result(500);
    }

    execute_interceptors_with_(intercept_policy_after::value, req, resp);

    write_log_for_(req, resp);

    return status;
  }

 private:
  bool dispatch_not_found_(response& resp) {
    resp.result(http::status::not_found);
    resp.set(http::field::content_type, "text/html");
    boost::beast::ostream(resp.body()) << "<h2>404 - Not Found</h2>";
    return true;
  }

  bool dispatch_method_not_allow_(response& resp) {
    resp.result(http::status::method_not_allowed);
    return true;
  }

  bool install_fn_handler_(http::verb method,
                           std::string_view endpoint,
                           handler_fn_type h_fn) {
    if (has_object_handler_for_(endpoint)) {
      LOG(ERROR) << "There is an object handler for [" << method << " "
                 << endpoint << "]" << std::endl;
      return false;
    }

    return handler_fn_registry_.register_handler(method, endpoint, h_fn);
  }

  bool install_object_handler_(std::string_view endpoint, handler_type& h_obj) {
    if (has_at_least_one_function_handler_for_(endpoint)) {
      LOG(ERROR) << "There is at least one handler for [" << endpoint << "]"
                 << std::endl;
      return false;
    }

    if (has_object_handler_for_(endpoint)) {
      // The handler is set and overwriting a handler is likely and error log
      // the error and fail.
      return emit_overwrite_error_(std::nullopt, endpoint);
    }

    // By policy, an object handler handles GET, POST, PUT and DELETE and the
    // regitry implementation knows that, we don't need to specify a method.
    auto registered = handler_object_registry_.register_handler(
        endpoint, std::reference_wrapper(h_obj));

    if (!registered) {
      return emit_emplace_error_(std::nullopt, endpoint);
    }

    return true;
  }

  bool has_at_least_one_function_handler_for_(std::string_view endpoint) const {
    return handler_fn_registry_.has(http::verb::get, endpoint) ||
           handler_fn_registry_.has(http::verb::post, endpoint) ||
           handler_fn_registry_.has(http::verb::put, endpoint) ||
           handler_fn_registry_.has(http::verb::delete_, endpoint);
  }

  bool has_function_handler_for_endpoint_and_method_(std::string_view endpoint,
                                                     http::verb method) {
    return handler_fn_registry_.has(method, endpoint);
  }

  bool has_object_handler_for_(std::string_view endpoint) const {
    return handler_object_registry_.has(all_method, endpoint);
  }

  bool emit_overwrite_error_(std::optional<http::verb> method,
                             std::string_view endpoint) {
    // The handler is set and overwriting a handler is likely and error log
    // the error and fail.
    LOG(ERROR) << "Handler overwrite is not allow, endpoint: ["
               << (method ? http::to_string(method.value()) : "all") << " "
               << endpoint << "]" << std::endl;
    return false;
  }

  bool emit_emplace_error_(std::optional<http::verb> method,
                           std::string_view endpoint) {
    // The handler is set and overwriting a handler is likely and error log
    // the error and fail.
    LOG(ERROR) << "Failed to emplace in dispatch table for ["
               << (method ? http::to_string(method.value()) : "all") << " "
               << endpoint << "]" << std::endl;
    return false;
  }

  void execute_interceptors_with_(interception_policy policy,
                                  const request& req,
                                  response& resp) {
    auto policy_filter = [policy](const auto& pair) {
      return pair.first == policy;
    };

    auto executor = [&](const auto& pair) { pair.second(req, resp); };

    for_each(interceptors_ | filtered(policy_filter), executor);
  }

  bool dispatch_with_(handler_type& object,
                      const request& req,
                      response& resp) {
    auto method = req.method();
    switch (method) {
      case http::verb::get:
        return object.get(req, resp);
      case http::verb::post:
        return object.post(req, resp);
      case http::verb::put:
        return object.put(req, resp);
      case http::verb::delete_:
        return object.del(req, resp);
      default:
        LOG(ERROR) << "Unsupported method " << method << std::endl;
        return false;
    }
  }

  bool dispatch_with_(handler_fn_type h_fn,
                      const request& req,
                      response& resp) {
    // TODO: We can do some post processing here instead of return
    return h_fn(req, resp);
  }

  void write_log_for_(const request& req, const response& resp) {
    // TODO: We can do better here to honest. There might be a standard way to
    // do this
    std::time_t now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string date_time(30, '\0');
    std::strftime(date_time.data(), date_time.size(), "%Y-%m-%d %H:%M:%S",
                  std::localtime(&now));

    LOG(INFO) << req.peer() << " - - [" << date_time << "] " << req.method()
              << " " << req.target() << " " << req.version() << " "
              << resp.result() << std::endl;
  }

 private:
  // TODO: Could this be a std::multimap? Need to investigate if/how that
  // would work
  handler_registry<handler_fn_type> handler_fn_registry_;
  handler_registry<handler_type> handler_object_registry_;

  std::forward_list<std::pair<interception_policy, interceptor_type>>
      interceptors_;
};
};  // namespace eagle

#endif  // EAGLE_DISPATCHER_HPP
