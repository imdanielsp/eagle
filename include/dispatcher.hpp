#pragma once

#include <array>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <chrono>
#include <forward_list>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

#include "common.hpp"
#include "handler.hpp"

using namespace boost::adaptors;
using namespace boost::range;

namespace eagle {

class base_dispatcher {
 public:
  base_dispatcher() {}
  virtual ~base_dispatcher() = default;

  virtual bool add_handler(http::verb method,
                           const std::string& endpoint,
                           handler_fn_type h_fn) = 0;

  virtual bool add_handler(const std::string& endpoint,
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
                   const std::string& endpoint,
                   handler_fn_type h_fn) override {
    return install_fn_handler_(method, endpoint, h_fn);
  }

  bool add_handler(const std::string& endpoint,
                   stateful_handler& h_obj) override {
    return install_object_handler_(endpoint, h_obj);
  }

  bool dispatch(const request& req, response& resp) override {
    execute_interceptors_with_(intercept_policy_before::value, req, resp);

    bool status = true;
    auto target_endpoint = std::string{req.target()};
    if (has_object_handler_for_(target_endpoint)) {
      auto object = dispatch_object_table_.find(target_endpoint)->second;
      status = dispatch_with_(object, req, resp);
    } else if (has_at_least_one_function_handler_for_(target_endpoint)) {
      const auto& handler_list =
          dispatch_fn_table_.find(target_endpoint)->second;
      status = dispatch_with_(handler_list, req, resp);
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
  // Allow allow  (GET, POST, PUT, and DELETE)
  enum supported_method_idx : size_t { get = 0, post, put, del, count };

  bool dispatch_not_found_(response& resp) {
    resp.result(http::status::not_found);
    return true;
  }

  bool dispatch_method_not_allow_(response& resp) {
    resp.result(http::status::method_not_allowed);
    return true;
  }

  bool install_fn_handler_(http::verb method,
                           const std::string& endpoint,
                           handler_fn_type h_fn) {
    if (has_object_handler_for_(endpoint)) {
      LOG(ERROR) << "There is an object handler for [" << method << " "
                 << endpoint << "]" << std::endl;
      return false;
    }

    auto itr = dispatch_fn_table_.find(endpoint);
    auto method_idx = dispatcher::get_index_for_verb_(method);

    if (itr != dispatch_fn_table_.end()) {
      if (itr->second[method_idx].second) {
        // The handler is set and overwriting a handler is likely and error log
        // the error and fail.
        return emit_overwrite_error_(method, endpoint);
      }

      itr->second[method_idx].second = h_fn;
    } else {
      std::array<std::pair<http::verb, handler_fn_type>,
                 supported_method_idx::count>
          handler_pair_list;
      handler_pair_list[method_idx] = std::make_pair(method, h_fn);
      auto [_, emplaced] = dispatch_fn_table_.try_emplace(
          endpoint, std::move(handler_pair_list));

      if (!emplaced) {
        return emit_emplace_error_(method, endpoint);
      }
    }

    return true;
  }

  bool install_object_handler_(const std::string& endpoint,
                               stateful_handler& h_obj) {
    if (has_at_least_one_function_handler_for_(endpoint)) {
      LOG(ERROR) << "There is at least one handler for [" << endpoint << "]"
                 << std::endl;
      return false;
    }

    auto itr = dispatch_object_table_.find(endpoint);
    if (itr != dispatch_object_table_.end()) {
      // The handler is set and overwriting a handler is likely and error log
      // the error and fail.
      return emit_overwrite_error_(std::nullopt, endpoint);
    }

    auto [_, emplaced] = dispatch_object_table_.try_emplace(endpoint, h_obj);

    if (!emplaced) {
      return emit_emplace_error_(std::nullopt, endpoint);
    }

    return true;
  }

  bool has_at_least_one_function_handler_for_(
      const std::string& endpoint) const {
    return dispatch_fn_table_.find(endpoint) != dispatch_fn_table_.end();
  }

  bool has_object_handler_for_(const std::string& endpoint) const {
    return dispatch_object_table_.find(endpoint) !=
           dispatch_object_table_.end();
  }

  static supported_method_idx get_index_for_verb_(http::verb method) {
    switch (method) {
      case http::verb::get:
        return supported_method_idx::get;
        break;
      case http::verb::post:
        return supported_method_idx::post;
        break;
      case http::verb::put:
        return supported_method_idx::put;
        break;
      case http::verb::delete_:
        return supported_method_idx::del;
        break;
      default:
        LOG(ERROR) << "Unsupported method index " << method << std::endl;
        assert(false);
        break;
    }
  }

  bool emit_overwrite_error_(std::optional<http::verb> method,
                             const std::string& endpoint) {
    // The handler is set and overwriting a handler is likely and error log
    // the error and fail.
    LOG(ERROR) << "Handler overwrite is not allow, endpoint: ["
               << (method ? http::to_string(method.value()) : "all") << " "
               << endpoint << "]" << std::endl;
    return false;
  }

  bool emit_emplace_error_(std::optional<http::verb> method,
                           const std::string& endpoint) {
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
    auto handler_idx = dispatcher::get_index_for_verb_(req.method());

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
  std::unordered_map<std::string,
                     std::array<std::pair<http::verb, handler_fn_type>,
                                supported_method_idx::count>>
      dispatch_fn_table_;

  std::unordered_map<std::string, std::reference_wrapper<stateful_handler>>
      dispatch_object_table_;

  std::forward_list<std::pair<interception_policy, interceptor_type>>
      interceptors_;
};
};  // namespace eagle
