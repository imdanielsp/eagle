#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "resource_matcher.hpp"

TEST(descriptorstring, CanonicalView) {
  eagle::descriptor_list ts;
  EXPECT_EQ(ts.canonical_view(), "[]");

  ts.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                    "path"});
  EXPECT_EQ(ts.canonical_view(), "[(S, 'path')]");
  EXPECT_FALSE(ts.has_dynamic_descriptor());

  ts.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "id",
      eagle::resource_descriptor_value_type::kinteger});
  EXPECT_EQ(ts.canonical_view(), "[(S, 'path'), (D, 'id', integer)]");
  EXPECT_TRUE(ts.has_dynamic_descriptor());

  ts.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "query",
      eagle::resource_descriptor_value_type::kstring});
  EXPECT_EQ(ts.canonical_view(),
            "[(S, 'path'), (D, 'id', integer), (D, 'query', string)]");
}

TEST(descriptorstring, PathView) {
  eagle::descriptor_list ts;
  EXPECT_EQ(ts.path_view(), "/");

  ts.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                    "path"});
  EXPECT_EQ(ts.path_view(), "/path");

  ts.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "id",
      eagle::resource_descriptor_value_type::kinteger});
  EXPECT_EQ(ts.path_view(), "/path/{integer:id}");

  ts.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "query",
      eagle::resource_descriptor_value_type::kstring});
  EXPECT_EQ(ts.path_view(), "/path/{integer:id}/{string:query}");
}

TEST(descriptorstring, OperatorEqual) {
  eagle::descriptor_list a;
  a.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                   "path"});
  a.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "id",
      eagle::resource_descriptor_value_type::kinteger});
  a.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "query",
      eagle::resource_descriptor_value_type::kstring});

  eagle::descriptor_list b;
  b.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                   "path"});
  b.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "id",
      eagle::resource_descriptor_value_type::kinteger});
  b.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "query",
      eagle::resource_descriptor_value_type::kstring});

  EXPECT_TRUE(a == b);

  b.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "query",
      eagle::resource_descriptor_value_type::kstring});
  EXPECT_FALSE(a == b);
}

TEST(descriptorstring, OperatorNotEqual) {
  eagle::descriptor_list a;
  a.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                   "path"});
  a.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "id",
      eagle::resource_descriptor_value_type::kinteger});
  a.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "query",
      eagle::resource_descriptor_value_type::kstring});

  eagle::descriptor_list b;
  b.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                   "path"});
  b.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "id",
      eagle::resource_descriptor_value_type::kinteger});
  b.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "query",
      eagle::resource_descriptor_value_type::kstring});

  EXPECT_FALSE(a != b);

  b.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "query",
      eagle::resource_descriptor_value_type::kstring});
  EXPECT_TRUE(a != b);
}

TEST(PathScanner, ScanStaticAndDynamicPath) {
  {
    eagle::path_scanner lex{"/"};

    auto descriptors = lex.scan();
    EXPECT_FALSE(lex.error());
    EXPECT_GT(descriptors.size(), 0);
  }

  {
    eagle::path_scanner lex{"/path/{integer:id}"};

    auto descriptors = lex.scan();
    EXPECT_FALSE(lex.error());

    eagle::descriptor_list ts;
    ts.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                      "path"});
    ts.add(eagle::resource_descriptor{
        eagle::resource_descriptor_type::kdynamic, "id",
        eagle::resource_descriptor_value_type::kinteger});

    EXPECT_EQ(ts, descriptors);
  }
}

TEST(PathScanner, ScanPathWithInvalidSymbol) {
  {
    eagle::path_scanner lex{"/path/{{integer:id}"};

    auto descriptors = lex.scan();
    EXPECT_TRUE(lex.error());
  }

  {
    eagle::path_scanner lex{"/path/{integer:id}}"};

    auto descriptors = lex.scan();
    EXPECT_TRUE(lex.error());
  }
}

TEST(PathScanner, ScanLongerStaticAndDynamicPath) {
  eagle::path_scanner lex{
      "/api/user/{integer:id}/{string:query}/more/{integer:test}"};

  auto descriptors = lex.scan();
  EXPECT_FALSE(lex.error());

  eagle::descriptor_list ts;
  ts.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                    "api"});
  ts.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                    "user"});
  ts.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "id",
      eagle::resource_descriptor_value_type::kinteger});
  ts.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "query",
      eagle::resource_descriptor_value_type::kstring});
  ts.add(eagle::resource_descriptor{eagle::resource_descriptor_type::kstatic,
                                    "more"});
  ts.add(eagle::resource_descriptor{
      eagle::resource_descriptor_type::kdynamic, "test",
      eagle::resource_descriptor_value_type::kinteger});

  EXPECT_EQ(ts, descriptors);
}

TEST(PathScanner, ScanUnsupportedValueType) {
  eagle::path_scanner lex{"/api/user/{badtype:id}"};

  auto descriptors = lex.scan();
  EXPECT_TRUE(lex.error());
}
