#include <gtest/gtest.h>

#include "params.hpp"

TEST(ParamsTest, ParamsSetAndGet) {
  eagle::params parameters;
  parameters.set("test", 1234);
  parameters.set<std::string_view>("test2", "string_value");

  EXPECT_EQ(parameters.get<int>("test"), 1234);
  EXPECT_EQ(parameters.get<std::string_view>("test2"), "string_value");
}

TEST(ParamsTest, ParamsExceptions) {
  eagle::params parameters;
  parameters.set("id", 1234);
  parameters.set<std::string_view>("query", "test");

  EXPECT_EQ(parameters.get<int>("id"), 1234);
  EXPECT_EQ(parameters.get<std::string_view>("query"), "test");

  EXPECT_THROW(parameters.get<int>("query"), eagle::invalid_param_cast);
  EXPECT_THROW(parameters.get<std::string_view>("id"),
               eagle::invalid_param_cast);
  EXPECT_THROW(parameters.get<int>("random"), eagle::param_not_found);
  EXPECT_THROW(parameters.get<std::string_view>("random"),
               eagle::param_not_found);
}
