#include <gtest/gtest.h>

#include "handler_registry.hpp"

TEST(HandlerRegistryTest, RegisterFunctionHandler) {
  eagle::handler_registry<eagle::handler_fn_type> registry;

  auto result = registry.register_handler(
      http::verb::get, "/some-endpoint",
      [](const auto&, auto&) -> bool { return true; });
  EXPECT_TRUE(result);
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

  auto [opt_handler, status] = registry.get_handler_for("/some-endpoint");
  EXPECT_TRUE(status);
  EXPECT_TRUE(opt_handler.has_value());
}

TEST(HandlerRegistryTest, GetUnRegisteredHandler) {
  eagle::handler_registry<eagle::handler_fn_type> registry;
  auto [opt_handler, status] = registry.get_handler_for("/some-endpoint");
  EXPECT_FALSE(status);
  EXPECT_FALSE(opt_handler.has_value());
}
