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
#include "request_arguments.hpp"
#include "resource_matcher.hpp"

namespace eagle {

using std::optional;
using std::pair;
using std::string_view;

inline constexpr std::nullopt_t all_method{std::nullopt};

// Allow allow  (GET, POST, PUT, and DELETE)
enum supported_method : size_t { get = 0, post, put, del, count, invalid };

namespace detail {

template <typename H>
struct is_handler : std::false_type {};

template <>
struct is_handler<handler_type> : std::true_type {};

template <>
struct is_handler<handler_fn_type> : std::true_type {};

inline constexpr supported_method get_index_for_verb(http::verb method) {
  switch (method) {
    case http::verb::get:
      return supported_method::get;
    case http::verb::post:
      return supported_method::post;
    case http::verb::put:
      return supported_method::put;
    case http::verb::delete_:
      return supported_method::del;
    default:
      LOG(ERROR) << "Unsupported method index " << method << std::endl;
      return supported_method::invalid;
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

namespace {
std::tuple<request_arguments, std::string, bool> match_resource_descriptors(
    const descriptor_list& ts,
    const std::vector<std::string_view>& partials) {
  request_arguments parameters;
  if (ts.size() != partials.size()) {
    return std::make_tuple(parameters, "", false);
  }

  for (size_t idx = 0; idx < ts.size(); idx++) {
    auto tkn = ts.at(idx);
    if (tkn.type == resource_descriptor_type::kdynamic) {
      if (tkn.value_type == resource_descriptor_value_type::kinteger) {
        try {
          auto value = std::stoi(partials[idx].data());
          parameters.set<int>(tkn.identifier, value);
        } catch (...) {
          return std::make_tuple(parameters, "", false);
        }
      } else if (tkn.value_type == resource_descriptor_value_type::kstring) {
        parameters.set<std::string_view>(tkn.identifier, partials[idx]);
      }
    }
  }

  return std::make_tuple(
      parameters, std::string(ts.path_view().data(), ts.path_view().size()),
      true);
}

std::vector<std::string_view> split_endpoint(std::string_view endpoint) {
  std::vector<std::string_view> partials;

  size_t start = 1;
  for (size_t end = start; end < endpoint.size(); end++) {
    if (endpoint.at(end) == '/') {
      partials.push_back(endpoint.substr(start, end - 1));
      start = end + 1;
    }
  }

  if (start != endpoint.size()) {
    partials.push_back(endpoint.substr(start, endpoint.size()));
  }

  return partials;
}
template <typename ContainerType>
std::tuple<request_arguments, std::string, bool> match_endpoint(
    std::string_view endpoint,
    const ContainerType& dispatch_table) {
  auto path_partials = split_endpoint(endpoint);
  for (const auto& [key, value] : dispatch_table) {
    path_scanner scanner{key};
    auto descriptors = scanner.scan();

    if (descriptors.has_dynamic_descriptor()) {
      auto [parameters, actual_endpoint, matched] =
          match_resource_descriptors(descriptors, path_partials);
      if (matched) {
        return std::make_tuple(parameters, actual_endpoint, matched);
      }
    }
  }

  return std::make_tuple(request_arguments(),
                         std::string(endpoint.data(), endpoint.size()), false);
}
}  // namespace

template <typename H>
struct handler_trait {};

template <>
struct handler_trait<handler_type> {
  using container_type =
      std::unordered_map<std::string_view,
                         std::reference_wrapper<handler_type>>;
  using value_type = container_type::mapped_type;

  struct impl {
    container_type dispatch_table_;

    bool register_handler(std::string_view endpoint,
                          std::reference_wrapper<handler_type> handler_ref) {
      path_scanner scanner{endpoint};
      auto descriptors = scanner.scan();

      if (scanner.error()) {
        return false;
      }

      auto itr = dispatch_table_.find(endpoint);
      if (itr != dispatch_table_.end()) {
        // The handler is set and overwriting a handler is likely and error log
        // the error and fail.
        return emit_overwrite_error(std::nullopt, endpoint);
      }

      auto [_, emplaced] = dispatch_table_.try_emplace(endpoint, handler_ref);

      if (!emplaced) {
        return emit_emplace_error(std::nullopt, endpoint);
      }

      return true;
    }

    pair<optional<value_type>, bool> get_handler_for(
        optional<http::verb> method,
        std::string_view endpoint,
        request_arguments* pParams) const {
      auto [parameters, actual_endpoint, matched] =
          match_endpoint(endpoint, dispatch_table_);

      auto itr = dispatch_table_.find(actual_endpoint);
      if (itr != dispatch_table_.end()) {
        if (pParams) {
          *pParams = parameters;
        }

        return std::make_pair(itr->second, true);
      }
      return std::make_pair(std::nullopt, false);
    }

    bool has(optional<http::verb> method, std::string_view endpoint) const {
      auto [_, found_handler] = get_handler_for(method, endpoint, nullptr);
      return found_handler;
    }
  };
};

template <>
struct handler_trait<handler_fn_type> {
  using container_type =
      std::unordered_map<std::string_view,
                         std::array<handler_fn_type, supported_method::count>>;
  using value_type = container_type::mapped_type::value_type;

  struct impl {
    container_type dispatch_table_;

    bool register_handler(http::verb method,
                          std::string_view endpoint,
                          handler_fn_type handler) {
      path_scanner scanner{endpoint};
      auto descriptors = scanner.scan();

      if (scanner.error()) {
        return false;
      }

      auto itr = dispatch_table_.find(endpoint);
      auto method_idx = get_index_for_verb(method);

      if (itr != dispatch_table_.end()) {
        if (itr->second[method_idx]) {
          // The handler is set and overwriting a handler is likely an error by
          // policy log the error and fail.
          return emit_overwrite_error(method, endpoint);
        }

        itr->second[method_idx] = handler;
      } else {
        // Create a list of handler and insert it in the map. The list of
        // handler has at most supported_method::count default-initialized
        // handlers which are in a invalid state allowing for
        // handler_fn_type::operator bool() semantics
        std::array<handler_fn_type, supported_method::count> handler_list;
        handler_list[method_idx].swap(handler);
        auto [_, emplaced] =
            dispatch_table_.try_emplace(endpoint, std::move(handler_list));

        if (!emplaced) {
          return emit_emplace_error(method, endpoint);
        }
      }

      return true;
    }

    pair<optional<value_type>, bool> get_handler_for(
        optional<http::verb> method,
        std::string_view endpoint,
        request_arguments* pParams) const {
      auto [parameters, actual_endpoint, matched] =
          match_endpoint(endpoint, dispatch_table_);

      auto itr = dispatch_table_.find(actual_endpoint);
      if (itr == dispatch_table_.end()) {
        return std::make_pair(std::nullopt, false);
      }

      if (pParams) {
        *pParams = parameters;
      }

      auto handler_idx =
          get_index_for_verb(method.value_or(http::verb::unknown));

      if (handler_idx == invalid) {
        return std::make_pair(std::nullopt, false);
      }

      auto handler = itr->second[handler_idx];

      if (!handler) {
        return std::make_pair(std::nullopt, false);
      }

      return std::make_pair(handler, true);
    }

    bool has(optional<http::verb> method, std::string_view endpoint) const {
      // If we are looking for all_method, we need to iterate over the
      // supported_method::count handlers. This could be simplified is a
      // std::multimap is used.
      if (method == all_method && dispatch_table_.count(endpoint) > 0) {
        auto handler_list = dispatch_table_.find(endpoint)->second;

        for (const auto& handler : handler_list) {
          if (!handler) {
            return false;
          }
        }

        return true;
      }

      // We are looking for one particular handler (method, endpoint)
      auto [_, found_handler] = get_handler_for(method, endpoint, nullptr);
      return found_handler;
    }
  };
};

}  // namespace detail

/// In memory storage of the request handler functions and handler objects
template <typename Handler>
class handler_registry {
 public:
  using impl = typename detail::handler_trait<Handler>::impl;
  using value_type = typename detail::handler_trait<Handler>::value_type;

  static_assert(detail::is_handler<Handler>::value,
                "Handler requirements are not met");

 public:
  handler_registry() = default;
  ~handler_registry() = default;

  bool register_handler(http::verb method,
                        std::string_view endpoint,
                        Handler handler) {
    return impl_.register_handler(method, endpoint, handler);
  }

  bool register_handler(std::string_view endpoint,
                        std::reference_wrapper<Handler> handler_ref) {
    return impl_.register_handler(endpoint, handler_ref);
  }

  pair<optional<value_type>, bool> get_handler_for(
      optional<http::verb> method,
      std::string_view endpoint,
      request_arguments& parameters) const {
    return impl_.get_handler_for(method, endpoint, &parameters);
  }

  bool has(optional<http::verb> method, std::string_view endpoint) const {
    return impl_.has(method, endpoint);
  }

 private:
  impl impl_;
};
}  // namespace eagle

#endif  // EAGLE_HANDLER_REGISTRY_HPP
