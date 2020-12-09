#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dispatcher.hpp"

using ::testing::_;

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

class HandlerMock : public eagle::stateful_handler {
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
  auto result =
      dispatcher_.add_handler(http::verb::get, "/endpoint",
                              [](const auto& req, auto& resp) { return true; });
  EXPECT_TRUE(result);

  result = dispatcher_.dispatch(request_, response_);

  EXPECT_TRUE(result);
  ASSERT_EQ(response_.result(), http::status::ok);
}

TEST_F(DispatcherTest, DispatchPostHandlerFn) {
  request_.method(http::verb::post);
  auto result =
      dispatcher_.add_handler(http::verb::post, "/endpoint",
                              [](const auto& req, auto& resp) { return true; });
  EXPECT_TRUE(result);

  result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);

  ASSERT_EQ(response_.result(), http::status::ok);
}

TEST_F(DispatcherTest, DispatchPutHandlerFn) {
  request_.method(http::verb::put);
  auto result =
      dispatcher_.add_handler(http::verb::put, "/endpoint",
                              [](const auto& req, auto& resp) { return true; });
  EXPECT_TRUE(result);

  result = dispatcher_.dispatch(request_, response_);

  EXPECT_TRUE(result);
  ASSERT_EQ(response_.result(), http::status::ok);
}

TEST_F(DispatcherTest, DispatchDeleteHandlerFn) {
  request_.method(http::verb::delete_);
  auto result =
      dispatcher_.add_handler(http::verb::delete_, "/endpoint",
                              [](const auto& req, auto& resp) { return true; });
  EXPECT_TRUE(result);

  result = dispatcher_.dispatch(request_, response_);

  EXPECT_TRUE(result);
  ASSERT_EQ(response_.result(), http::status::ok);
}

// TODO: Split these test, one for each HTTP method supported
TEST_F(DispatcherTest, DispatchWithHandlerObject) {
  request_.target("/api/v1/users");

  HandlerMock users_api;
  auto result =
      dispatcher_.add_handler(std::string{request_.target()}, users_api);
  EXPECT_TRUE(result);

  request_.method(http::verb::get);
  EXPECT_CALL(users_api, get(_, _))
      .Times(testing::AtLeast(1))
      .WillOnce(testing::Return(true));
  result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);
  EXPECT_EQ(response_.result(), http::status::ok);

  request_.method(http::verb::post);
  EXPECT_CALL(users_api, post(_, _))
      .Times(testing::AtLeast(1))
      .WillOnce(testing::Return(true));
  result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);
  EXPECT_EQ(response_.result(), http::status::ok);

  request_.method(http::verb::put);
  EXPECT_CALL(users_api, put(_, _))
      .Times(testing::AtLeast(1))
      .WillOnce(testing::Return(true));
  result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);
  EXPECT_EQ(response_.result(), http::status::ok);

  request_.method(http::verb::delete_);
  EXPECT_CALL(users_api, del(_, _))
      .Times(testing::AtLeast(1))
      .WillOnce(testing::Return(true));
  result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);
  EXPECT_EQ(response_.result(), http::status::ok);
}

TEST_F(DispatcherTest, NotFound) {
  auto result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);

  ASSERT_EQ(response_.result(), http::status::not_found);
}

TEST_F(DispatcherTest, MethodNotAllow) {
  auto result =
      dispatcher_.add_handler(http::verb::get, "/endpoint",
                              [](const auto& req, auto& resp) { return true; });
  EXPECT_TRUE(result);

  request_.method(http::verb::post);
  result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);

  ASSERT_EQ(response_.result(), http::status::method_not_allowed);
}

TEST_F(DispatcherTest, HandlerReturnFalseYieldInternalError) {
  auto result = dispatcher_.add_handler(
      http::verb::get, "/endpoint",
      [](const auto& req, auto& resp) { return false; });
  EXPECT_TRUE(result);

  result = dispatcher_.dispatch(request_, response_);
  EXPECT_FALSE(result);

  ASSERT_EQ(response_.result(), http::status::internal_server_error);
}