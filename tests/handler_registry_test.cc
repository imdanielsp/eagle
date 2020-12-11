#include <gtest/gtest.h>

#include "handler_registry.hpp"
#include "test_utils.hpp"

TEST(HandlerRegistryTest, RegisterFunctionHandler) {
  eagle::handler_registry<eagle::handler_fn_type> registry;

  auto result = registry.register_handler(
      http::verb::get, "/some-endpoint",
      [](const auto&, auto&) -> bool { return true; });
  EXPECT_TRUE(result);
}

TEST(HandlerRegistryTest, RegisterFunctionHandlersForSameURI) {
  eagle::handler_registry<eagle::handler_fn_type> registry;
  {
    auto result = registry.register_handler(
        http::verb::get, "/some-endpoint",
        [](const auto&, auto&) -> bool { return true; });
    EXPECT_TRUE(result);

    auto [opt_handler, status] =
        registry.get_handler_for(http::verb::get, "/some-endpoint");
    EXPECT_TRUE(status);
    EXPECT_TRUE(opt_handler.has_value());
  }

  {
    auto result = registry.register_handler(
        http::verb::post, "/some-endpoint",
        [](const auto&, auto&) -> bool { return true; });
    EXPECT_TRUE(result);

    auto [opt_handler, status] =
        registry.get_handler_for(http::verb::post, "/some-endpoint");
    EXPECT_TRUE(status);
    EXPECT_TRUE(opt_handler.has_value());
  }

  {
    auto result = registry.register_handler(
        http::verb::put, "/some-endpoint",
        [](const auto&, auto&) -> bool { return true; });
    EXPECT_TRUE(result);

    auto [opt_handler, status] =
        registry.get_handler_for(http::verb::put, "/some-endpoint");
    EXPECT_TRUE(status);
    EXPECT_TRUE(opt_handler.has_value());
  }

  {
    auto result = registry.register_handler(
        http::verb::delete_, "/some-endpoint",
        [](const auto&, auto&) -> bool { return true; });
    EXPECT_TRUE(result);

    auto [opt_handler, status] =
        registry.get_handler_for(http::verb::delete_, "/some-endpoint");
    EXPECT_TRUE(status);
    EXPECT_TRUE(opt_handler.has_value());
  }
}

TEST(HandlerRegistryTest, RegisterFunctionHandlerTwiceForSameEndpoint) {
  eagle::handler_registry<eagle::handler_fn_type> registry;

  auto result = registry.register_handler(
      http::verb::get, "/some-endpoint",
      [](const auto&, auto&) -> bool { return true; });
  EXPECT_TRUE(result);

  result = registry.register_handler(
      http::verb::get, "/some-endpoint",
      [](const auto&, auto&) -> bool { return true; });
  EXPECT_FALSE(result);
}

TEST(HandlerRegistryTest, GetRegisteredHandler) {
  eagle::handler_registry<eagle::handler_fn_type> registry;

  auto result = registry.register_handler(
      http::verb::get, "/some-endpoint",
      [](const auto&, auto&) -> bool { return true; });
  EXPECT_TRUE(result);

  {
    auto [opt_handler, status] =
        registry.get_handler_for(http::verb::get, "/some-endpoint");
    EXPECT_TRUE(status);
    EXPECT_TRUE(opt_handler.has_value());
  }
  {
    auto [opt_handler, status] =
        registry.get_handler_for(http::verb::post, "/some-endpoint");
    EXPECT_FALSE(status);
    EXPECT_FALSE(opt_handler.has_value());
  }
}

TEST(HandlerRegistryTest, GetUnRegisteredFunctionHandler) {
  eagle::handler_registry<eagle::handler_fn_type> registry;
  auto [opt_handler, status] =
      registry.get_handler_for(http::verb::get, "/some-endpoint");
  EXPECT_FALSE(status);
  EXPECT_FALSE(opt_handler.has_value());
}

TEST(HandlerRegistryTest, GetUnsupportedMethodForFunctionHandler) {
  eagle::handler_registry<eagle::handler_fn_type> registry;
  {
    auto result = registry.register_handler(
        http::verb::get, "/some-endpoint",
        [](const auto&, auto&) -> bool { return true; });
    EXPECT_TRUE(result);
  }

  auto [opt_handler, result] =
      registry.get_handler_for(http::verb::head, "/some-endpoint");
  EXPECT_FALSE(result);
  EXPECT_FALSE(opt_handler.has_value());
}

TEST(HandlerRegistryTest, RegisterObjectHandler) {
  eagle::handler_registry<eagle::handler_type> registry;
  HandlerMock mock_handler;

  auto result = registry.register_handler("/some-endpoint", mock_handler);
  EXPECT_TRUE(result);

  auto [opt_handler, status] =
      registry.get_handler_for(eagle::all_method, "/some-endpoint");
  EXPECT_TRUE(status);
  EXPECT_TRUE(opt_handler.has_value());
}

TEST(HandlerRegistryTest, RegisterObjectHandlerTwiceForSameEndpoint) {
  eagle::handler_registry<eagle::handler_type> registry;
  HandlerMock mock_handler;

  auto result = registry.register_handler("/some-endpoint", mock_handler);
  EXPECT_TRUE(result);

  result = registry.register_handler("/some-endpoint", mock_handler);
  EXPECT_FALSE(result);

  auto [opt_handler, status] =
      registry.get_handler_for(eagle::all_method, "/some-endpoint");
  EXPECT_TRUE(status);
  EXPECT_TRUE(opt_handler.has_value());
}

TEST(HandlerRegistryTest, GetUnRegisteredObjectHandler) {
  eagle::handler_registry<eagle::handler_type> registry;
  auto [opt_handler, status] =
      registry.get_handler_for(http::verb::get, "/some-endpoint");
  EXPECT_FALSE(status);
  EXPECT_FALSE(opt_handler.has_value());
}

TEST(HandlerRegistryTest, HasFunctionHandler) {
  eagle::handler_registry<eagle::handler_fn_type> registry;
  auto result = registry.register_handler(
      http::verb::get, "/some-endpoint",
      [](const auto&, auto&) -> bool { return true; });
  EXPECT_TRUE(result);
  EXPECT_TRUE(registry.has(http::verb::get, "/some-endpoint"));
  EXPECT_FALSE(registry.has(http::verb::post, "/some-endpoint"));
  EXPECT_FALSE(registry.has(eagle::all_method, "/some-endpoint"));
}

TEST(HandlerRegistryTest, HasObjectHandler) {
  eagle::handler_registry<eagle::handler_type> registry;
  HandlerMock mock_handler;

  auto result = registry.register_handler("/some-endpoint", mock_handler);
  EXPECT_TRUE(result);
  EXPECT_TRUE(registry.has(http::verb::get, "/some-endpoint"));
  EXPECT_TRUE(registry.has(http::verb::post, "/some-endpoint"));
  EXPECT_TRUE(registry.has(http::verb::put, "/some-endpoint"));
  EXPECT_TRUE(registry.has(http::verb::delete_, "/some-endpoint"));
  EXPECT_TRUE(registry.has(eagle::all_method, "/some-endpoint"));
}
