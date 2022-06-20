
#include <gtest/gtest.h>

#include "common.hpp"
#include "handler.hpp"

TEST(HandlerBaseTest, BaseHandlerResult) {
  eagle::stateful_handler_base handler;
  eagle::request req;
  eagle::response resp;

  resp.result(http::status::ok);
  EXPECT_TRUE(handler.get(req, resp));
  EXPECT_EQ(resp.result(), http::status::method_not_allowed);

  resp.result(http::status::ok);
  EXPECT_TRUE(handler.post(req, resp));
  EXPECT_EQ(resp.result(), http::status::method_not_allowed);

  resp.result(http::status::ok);
  EXPECT_TRUE(handler.put(req, resp));
  EXPECT_EQ(resp.result(), http::status::method_not_allowed);

  resp.result(http::status::ok);
  EXPECT_TRUE(handler.del(req, resp));
  EXPECT_EQ(resp.result(), http::status::method_not_allowed);
}
