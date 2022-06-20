#include <gtest/gtest.h>

#include "response.hpp"

TEST(ResponseTest, JsonToHtml) {
  eagle::response resp;

  EXPECT_NO_THROW(resp.json());
  EXPECT_NO_THROW(resp.json());
  EXPECT_EQ(resp.writer_type(), eagle::writer_type::kjson);
  EXPECT_THROW(resp.html(), eagle::invalid_writer_operation);
}

TEST(ResponseTest, HtmltoJson) {
  eagle::response resp;

  EXPECT_NO_THROW(resp.html());
  EXPECT_NO_THROW(resp.html());
  EXPECT_EQ(resp.writer_type(), eagle::writer_type::khtml);
  EXPECT_THROW(resp.json(), eagle::invalid_writer_operation);
}
