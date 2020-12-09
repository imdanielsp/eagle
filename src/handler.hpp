#pragma once

#include <glog/logging.h>
#include <functional>
#include <typeinfo>

#include "common.hpp"

namespace eagle {

class handler_object;

using handler_type = handler_object;
using handler_fn_type = std::function<bool(const request&, response&)>;

class handler_object {
 public:
  handler_object() = default;
  virtual ~handler_object() = default;

  virtual bool get(const request& req, response& resp) {
    LOG(WARNING) << "GET handler is not implemented for " << req.target()
                 << std::endl;

    resp.result(http::status::not_found);
    return false;
  }

  virtual bool post(const request& req, response& resp) {
    LOG(WARNING) << "POST handler is not implemented for " << req.target()
                 << std::endl;
    resp.result(http::status::not_found);
    return false;
  }

  virtual bool put(const request& req, response& resp) {
    LOG(WARNING) << "PUT handler is not implemented for " << req.target()
                 << std::endl;
    return false;
    resp.result(http::status::not_found);
  }

  virtual bool del(const request& req, response& resp) {
    LOG(WARNING) << "DELETE handler is not implemented for " << req.target()
                 << std::endl;
    resp.result(http::status::not_found);
    return false;
  }
};
}  // namespace eagle
