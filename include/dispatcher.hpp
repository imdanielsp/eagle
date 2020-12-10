#pragma once

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

class base_dispatcher {
 public:
  base_dispatcher() {}
  virtual ~base_dispatcher() = default;

  virtual bool add_handler(http::verb method,
                           std::string_view endpoint,
                           handler_fn_type h_fn) = 0;

  virtual bool add_handler(std::string_view endpoint,
                           stateful_handler& h_obj) = 0;

  virtual bool dispatch(const request& request, response& response) = 0;
};

class dispatcher : public base_dispatcher {
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

  bool add_handler(std::string_view endpoint,
                   stateful_handler& h_obj) override {
    return install_object_handler_(endpoint, h_obj);
  }

  bool dispatch(const request& req, response& resp) override {
    execute_interceptors_with_(intercept_policy_before::value, req, resp);

    bool status = true;
    auto target_endpoint = std::string{req.target()};
    if (has_object_handler_for_(target_endpoint)) {
      auto [opt_object, _] =
          handler_object_registry_.get_handler_for(target_endpoint);
      status = dispatch_with_(opt_object.value().get(), req, resp);
    } else if (has_at_least_one_function_handler_for_(target_endpoint)) {
      auto [opt_handler_list, _] =
          handler_fn_registry_.get_handler_for(target_endpoint);
      status = dispatch_with_(opt_handler_list.value(), req, resp);
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

  bool install_object_handler_(std::string_view endpoint,
                               stateful_handler& h_obj) {
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
    // regitry implementation knows that. We pass a std::nullopt because the
    // method is undefined
    auto registered = handler_object_registry_.register_handler(
        std::nullopt, endpoint, std::reference_wrapper(h_obj));

    if (!registered) {
      return emit_emplace_error_(std::nullopt, endpoint);
    }

    return true;
  }

  bool has_at_least_one_function_handler_for_(std::string_view endpoint) const {
    return handler_fn_registry_.has(endpoint);
  }

  bool has_object_handler_for_(std::string_view endpoint) const {
    return handler_object_registry_.has(endpoint);
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

  bool dispatch_with_(stateful_handler& object,
                      const request& req,
                      response& resp) {
    auto method = req.method();
    switch (method) {
      case http::verb::get:
        return object.get(req, resp);
        break;
      case http::verb::post:
        return object.post(req, resp);
        break;
      case http::verb::put:
        return object.put(req, resp);
        break;
      case http::verb::delete_:
        return object.del(req, resp);
        break;
      default:
        LOG(ERROR) << "Unsupported method " << method << std::endl;
        assert(false);
        break;
    }
  }

  bool dispatch_with_(
      const std::array<std::pair<http::verb, handler_fn_type>,
                       supported_method_idx::count>& handler_list,
      const request& req,
      response& resp) {
    auto handler_idx = eagle::detail::get_index_for_verb(req.method());

    if (!handler_list[handler_idx].second) {
      return dispatch_method_not_allow_(resp);
    }

    // TODO: We can do some post processing here instead of return
    return handler_list[handler_idx].second(req, resp);
  }

  void write_log_for_(const request& req, const response& resp) {
    // TODO: We can do better here to honest. There might be a standard way to
    // do this
    std::time_t now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string date_time(30, '\0');
    std::strftime(&date_time[0], date_time.size(), "%Y-%m-%d %H:%M:%S",
                  std::localtime(&now));

    LOG(INFO) << req.at(http::field::host) << " - - [" << date_time << "] "
              << req.method() << " " << req.target() << " " << req.version()
              << " " << resp.result() << std::endl;
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
