#ifndef EAGLE_HANDLER_REGISTRY_HPP
#define EAGLE_HANDLER_REGISTRY_HPP

#include <boost/beast.hpp>
#include <forward_list>
#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "handler.hpp"

namespace eagle {

// Allow allow  (GET, POST, PUT, and DELETE)
enum supported_method_idx : size_t { get = 0, post, put, del, count };

namespace detail {

template <typename H>
struct is_handler : std::false_type {};

template <>
struct is_handler<handler_type> : std::true_type {};

template <>
struct is_handler<handler_fn_type> : std::true_type {};

inline constexpr supported_method_idx get_index_for_verb(http::verb method) {
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

inline bool emit_overwrite_error(std::optional<http::verb> method,
                                 std::string_view endpoint) {
  // The handler is set and overwriting a handler is likely and error log
  // the error and fail.
  LOG(ERROR) << "Handler overwrite is not allow, endpoint: ["
             << (method ? http::to_string(method.value()) : "all") << " "
             << endpoint << "]" << std::endl;
  return false;
}

inline bool emit_emplace_error(std::optional<http::verb> method,
                               std::string_view endpoint) {
  // The handler is set and overwriting a handler is likely and error log
  // the error and fail.
  LOG(ERROR) << "Failed to emplace in dispatch table for ["
             << (method ? http::to_string(method.value()) : "all") << " "
             << endpoint << "]" << std::endl;
  return false;
}

template <typename H>
struct handler_trait {};

template <>
struct handler_trait<handler_type> {
  using container_type =
      std::unordered_map<std::string_view,
                         std::reference_wrapper<handler_type>>;
  using value_type = container_type::mapped_type;

  struct impl {
    static bool register_handler(
        container_type& container,
        http::verb method,
        std::string_view endpoint,
        std::reference_wrapper<handler_type> handler_ref) {
      (void)method;

      auto itr = container.find(endpoint);
      if (itr != container.end()) {
        // The handler is set and overwriting a handler is likely and error log
        // the error and fail.
        return emit_overwrite_error(std::nullopt, endpoint);
      }

      auto [_, emplaced] = container.try_emplace(endpoint, handler_ref);

      if (!emplaced) {
        return emit_emplace_error(std::nullopt, endpoint);
      }

      return true;
    }
  };
};

template <>
struct handler_trait<handler_fn_type> {
  using container_type =
      std::unordered_map<std::string_view,
                         std::array<std::pair<http::verb, handler_fn_type>,
                                    supported_method_idx::count>>;
  using value_type = container_type::mapped_type;

  struct impl {
    static bool register_handler(container_type& container,
                                 http::verb method,
                                 std::string_view endpoint,
                                 handler_fn_type handler) {
      auto itr = container.find(endpoint);
      auto method_idx = get_index_for_verb(method);

      if (itr != container.end()) {
        if (itr->second[method_idx].second) {
          // The handler is set and overwriting a handler is likely and error
          // log the error and fail.
          return emit_overwrite_error(method, endpoint);
        }

        itr->second[method_idx].second = handler;
      } else {
        std::array<std::pair<http::verb, handler_fn_type>,
                   supported_method_idx::count>
            handler_pair_list;
        handler_pair_list[method_idx] = std::make_pair(method, handler);
        auto [_, emplaced] =
            container.try_emplace(endpoint, std::move(handler_pair_list));

        if (!emplaced) {
          return emit_emplace_error(method, endpoint);
        }
      }

      return true;
    }
  };
};

}  // namespace detail

/// In memory storage of the request handler functions and handler objects
template <typename Handler>
class handler_registry {
 public:
  using impl = typename detail::handler_trait<Handler>::impl;

  static_assert(detail::is_handler<Handler>::value,
                "Handler requirements are not met");

 public:
  handler_registry() = default;
  ~handler_registry() = default;

  bool register_handler(std::optional<http::verb> method,
                        std::string_view endpoint,
                        Handler handler) {
    return impl::register_handler(dispatch_table_,
                                  method.value_or(http::verb::unknown),
                                  endpoint, handler);
  }

  bool register_handler(std::optional<http::verb> method,
                        std::string_view endpoint,
                        std::reference_wrapper<Handler> handler_ref) {
    return impl::register_handler(dispatch_table_,
                                  method.value_or(http::verb::unknown),
                                  endpoint, handler_ref);
  }

  std::pair<std::optional<typename detail::handler_trait<Handler>::value_type>,
            bool>
  get_handler_for(std::string_view endpoint) const {
    auto itr = dispatch_table_.find(endpoint);
    if (itr != dispatch_table_.end()) {
      return std::make_pair(itr->second, true);
    }
    return std::make_pair(std::nullopt, false);
  }

  bool has(std::string_view endpoint) const {
    return dispatch_table_.count(endpoint) > 0;
  }

 private:
  typename detail::handler_trait<Handler>::container_type dispatch_table_;
};
}  // namespace eagle

#endif  // EAGLE_HANDLER_REGISTRY_HPP
