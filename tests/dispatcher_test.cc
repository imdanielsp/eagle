#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dispatcher.hpp"
#include "test_utils.hpp"

using ::testing::_;

class DispatcherTest : public ::testing::Test {
 protected:
  void SetUp() override {
    request_.method(http::verb::get);
    request_.peer("localhost");
    request_.target("/endpoint");
  }

  eagle::request request_;
  eagle::response response_;
  eagle::dispatcher dispatcher_;
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

TEST_F(DispatcherTest, DispatchGetHandlerFnWithResourceId) {
  request_.method(http::verb::get);
  auto result = dispatcher_.add_handler(
      http::verb::get, "/user/{integer:id}/{string:query}",
      [](const auto& req, auto& resp) { return true; });
  EXPECT_TRUE(result);

  request_.target("/user/123/test");
  result = dispatcher_.dispatch(request_, response_);

  EXPECT_TRUE(result);
  EXPECT_EQ(response_.result(), http::status::ok);

  auto params = request_.params();
  EXPECT_EQ(params.get<int>("id"), 123);
  EXPECT_EQ(params.get<std::string_view>("query"), "test");
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

TEST_F(DispatcherTest, DispatchHandlerObjectWithResourceId) {
  request_.target("/api/v1/users/1234/test");

  HandlerMock users_api;
  auto result = dispatcher_.add_handler(
      "/api/v1/users/{integer:id}/{string:query}", users_api);
  EXPECT_TRUE(result);

  request_.method(http::verb::get);
  EXPECT_CALL(users_api, get(_, _))
      .Times(testing::AtLeast(1))
      .WillOnce(testing::Return(true));
  result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);
  EXPECT_EQ(response_.result(), http::status::ok);

  EXPECT_EQ(request_.params().get<int>("id"), 1234);
  EXPECT_EQ(request_.params().get<std::string_view>("query"), "test");

  EXPECT_THROW(request_.params().get<int>("query"), eagle::invalid_param_cast);
  EXPECT_THROW(request_.params().get<std::string_view>("id"),
               eagle::invalid_param_cast);
  EXPECT_THROW(request_.params().get<int>("random"), eagle::param_not_found);
  EXPECT_THROW(request_.params().get<std::string_view>("random"),
               eagle::param_not_found);
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

TEST_F(DispatcherTest, AddObjectHandlerWithFunctionHandler) {
  auto result = dispatcher_.add_handler(
      http::verb::get, "/endpoint",
      [](const auto& req, auto& resp) { return false; });
  EXPECT_TRUE(result);

  HandlerMock mockHandler;
  result = dispatcher_.add_handler("/endpoint", mockHandler);
  EXPECT_FALSE(result);
}

TEST_F(DispatcherTest, AddFunctionHandlerWithObjectHandler) {
  HandlerMock mockHandler;
  auto result = dispatcher_.add_handler("/endpoint", mockHandler);
  EXPECT_TRUE(result);

  result = dispatcher_.add_handler(
      http::verb::get, "/endpoint",
      [](const auto& req, auto& resp) { return false; });
  EXPECT_FALSE(result);
}

TEST_F(DispatcherTest, TryOvewriteObjectHandler) {
  HandlerMock mockHandler;
  auto result = dispatcher_.add_handler("/endpoint", mockHandler);
  EXPECT_TRUE(result);

  result = dispatcher_.add_handler("/endpoint", mockHandler);
  EXPECT_FALSE(result);
}

TEST_F(DispatcherTest, AddInterceptorAfter) {
  dispatcher_.add_interceptor(eagle::intercept_policy_after::value,
                              [](const auto& req, auto& resp) {
                                resp.result(http::status::bad_request);
                              });
  auto result = dispatcher_.add_handler(http::verb::get, "/endpoint",
                                        [](const auto& req, auto& resp) {
                                          resp.result(http::status::ok);
                                          return true;
                                        });
  EXPECT_TRUE(result);

  result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);

  EXPECT_EQ(response_.result(), http::status::bad_request);
}

TEST_F(DispatcherTest, AddInterceptorBefore) {
  dispatcher_.add_interceptor(eagle::intercept_policy_before::value,
                              [](const auto& req, auto& resp) {
                                resp.result(http::status::bad_request);
                              });
  auto result =
      dispatcher_.add_handler(http::verb::get, "/endpoint",
                              [](const auto& req, auto& resp) { return true; });
  EXPECT_TRUE(result);

  result = dispatcher_.dispatch(request_, response_);
  EXPECT_TRUE(result);

  EXPECT_EQ(response_.result(), http::status::bad_request);
}

TEST_F(DispatcherTest, UnsupportedMethod) {
  HandlerMock mockHandler;
  auto result = dispatcher_.add_handler("/endpoint", mockHandler);
  EXPECT_TRUE(result);

  request_.method(http::verb::move);
  result = dispatcher_.dispatch(request_, response_);
  EXPECT_FALSE(result);
}
