#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dispatcher.hpp"

class DispatcherTest : public ::testing::Test {
 protected:
  void SetUp() override {
    request_.method(http::verb::get);
    request_.set(http::field::host, "localhost");
    request_.target("/endpoint");
  }

  eagle::request request_;
  eagle::response response_;
  eagle::dispatcher dispatcher_;
};

class HandlerMock : eagle::stateful_handler {
 public:
  MOCK_METHOD(bool, get, (const eagle::request&, eagle::response&), (override));
  MOCK_METHOD(bool,
              post,
              (const eagle::request&, eagle::response&),
              (override));
  MOCK_METHOD(bool, put, (const eagle::request&, eagle::response&), (override));
  MOCK_METHOD(bool, del, (const eagle::request&, eagle::response&), (override));
};

TEST_F(DispatcherTest, DispatchGetHandlerFn) {
  request_.method(http::verb::get);
  dispatcher_.add_handler(http::verb::get, "/endpoint",
                          [](const auto& req, auto& resp) { return true; });

  auto result = dispatcher_.dispatch(request_, response_);

  EXPECT_TRUE(result);
  ASSERT_EQ(response_.result(), http::status::ok);
}

TEST_F(DispatcherTest, DispatchPostHandlerFn) {
  request_.method(http::verb::post);
  dispatcher_.add_handler(http::verb::post, "/endpoint",
                          [](const auto& req, auto& resp) { return true; });

  auto result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);

  ASSERT_EQ(response_.result(), http::status::ok);
}

TEST_F(DispatcherTest, DispatchPutHandlerFn) {
  request_.method(http::verb::put);
  dispatcher_.add_handler(http::verb::put, "/endpoint",
                          [](const auto& req, auto& resp) { return true; });

  auto result = dispatcher_.dispatch(request_, response_);

  EXPECT_TRUE(result);
  ASSERT_EQ(response_.result(), http::status::ok);
}

TEST_F(DispatcherTest, DispatchDeleteHandlerFn) {
  request_.method(http::verb::delete_);
  dispatcher_.add_handler(http::verb::delete_, "/endpoint",
                          [](const auto& req, auto& resp) { return true; });

  auto result = dispatcher_.dispatch(request_, response_);

  EXPECT_TRUE(result);
  ASSERT_EQ(response_.result(), http::status::ok);
}

TEST_F(DispatcherTest, DispatchWithHandlerObject) {

}

TEST_F(DispatcherTest, NotFound) {
  auto result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);

  ASSERT_EQ(response_.result(), http::status::not_found);
}

TEST_F(DispatcherTest, MethodNotAllow) {
  dispatcher_.add_handler(http::verb::get, "/endpoint",
                          [](const auto& req, auto& resp) { return true; });

  request_.method(http::verb::post);
  auto result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);

  ASSERT_EQ(response_.result(), http::status::method_not_allowed);
}

TEST_F(DispatcherTest, HandlerReturnFalseYieldInternalError) {
  dispatcher_.add_handler(http::verb::get, "/endpoint",
                          [](const auto& req, auto& resp) { return false; });

  auto result = dispatcher_.dispatch(request_, response_);
  EXPECT_FALSE(result);

  ASSERT_EQ(response_.result(), http::status::internal_server_error);
}