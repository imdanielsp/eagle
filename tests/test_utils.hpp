#ifndef EAGLE_TEST_UTILS_HPP
#define EAGLE_TEST_UTILS_HPP

#include <gmock/gmock.h>

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

#endif
