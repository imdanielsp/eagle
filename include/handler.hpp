#ifndef EAGLE_HANDLER_HPP
#define EAGLE_HANDLER_HPP

#include <glog/logging.h>
#include <functional>
#include <typeinfo>

#include "common.hpp"

namespace eagle {

class handler_interface;

using handler_type = handler_interface;
using handler_fn_type = std::function<bool(const request&, response&)>;

/// Handler interface for objects implementing the GET, POST, PUT, and DELETE
/// HTTP method. For a base implementation, see `eagle::stateful_handler_base`.
///
/// Note: if an object is implementing all HTTP method, it is recommended to
/// inherit directly from this pure virtual class.
class handler_interface {
 public:
  handler_interface() = default;

  virtual ~handler_interface() = default;

  virtual bool get(const request& req, response& resp) = 0;

  virtual bool post(const request& req, response& resp) = 0;

  virtual bool put(const request& req, response& resp) = 0;

  virtual bool del(const request& req, response& resp) = 0;
};

/// Base implementation of the `eagle::handler_interface` pure virtual class.
/// This class is a trivial implementation of a handler that set the HTTP status
/// to Method Not Allowed by default. Specialization of this class can override
/// zero or more of the methods.
class stateful_handler_base : public handler_interface {
 public:
  stateful_handler_base() = default;
  virtual ~stateful_handler_base() = default;

  bool get(const request& req, response& resp) override {
    LOG(WARNING) << "GET handler is not implemented for " << req.target()
                 << std::endl;

    resp.result(http::status::method_not_allowed);
    return true;
  }

  bool post(const request& req, response& resp) override {
    LOG(WARNING) << "POST handler is not implemented for " << req.target()
                 << std::endl;
    resp.result(http::status::method_not_allowed);
    return true;
  }

  bool put(const request& req, response& resp) override {
    LOG(WARNING) << "PUT handler is not implemented for " << req.target()
                 << std::endl;
    return true;
    resp.result(http::status::method_not_allowed);
  }

  bool del(const request& req, response& resp) override {
    LOG(WARNING) << "DELETE handler is not implemented for " << req.target()
                 << std::endl;
    resp.result(http::status::method_not_allowed);
    return true;
  }
};
}  // namespace eagle

#endif  // EAGLE_HANDLER_HPP
