#include <gtest/gtest.h>

#include "request_arguments.hpp"

TEST(RequestArgumentsTest, ArgumentsSetAndGet) {
  eagle::request_arguments parameters;
  parameters.set("test", 1234);
  parameters.set<std::string_view>("test2", "string_value");

  EXPECT_EQ(parameters.get<int>("test"), 1234);
  EXPECT_EQ(parameters.get<std::string_view>("test2"), "string_value");
}

TEST(RequestArgumentsTest, ArgumentsExceptions) {
  eagle::request_arguments parameters;
  parameters.set("id", 1234);
  parameters.set<std::string_view>("query", "test");

  EXPECT_EQ(parameters.get<int>("id"), 1234);
  EXPECT_EQ(parameters.get<std::string_view>("query"), "test");

  EXPECT_THROW(parameters.get<int>("query"), eagle::invalid_argument_cast);
  EXPECT_THROW(parameters.get<std::string_view>("id"),
               eagle::invalid_argument_cast);
  EXPECT_THROW(parameters.get<int>("random"), eagle::argument_not_found);
  EXPECT_THROW(parameters.get<std::string_view>("random"),
               eagle::argument_not_found);
}
