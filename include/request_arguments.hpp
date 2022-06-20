#ifndef EAGLE_REQUEST_ARGUMENTS_HPP
#define EAGLE_REQUEST_ARGUMENTS_HPP

#include <exception>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace eagle {

class argument_not_found : public std::exception {
 public:
  argument_not_found(std::string_view key) : key_(key.data(), key.size()) {}

  const char* what() const noexcept override {
    output_ = key_ + " is not in the parameters list";
    return output_.c_str();
  }

 private:
  mutable std::string output_;
  std::string key_;
};

class invalid_argument_cast : public std::exception {};

class request_arguments final {
 public:
  request_arguments() = default;
  ~request_arguments() = default;

  template <typename ValueType>
  const ValueType& get(std::string key) const {
    static_assert(std::is_same<ValueType, std::string_view>::value ||
                      std::is_same<ValueType, int>::value,
                  "Only strings and integers are supported");
    auto itr = arguments_.find(key);
    if (itr == arguments_.end()) {
      throw argument_not_found(key);
    }

    try {
      const auto& value = std::get<ValueType>(itr->second);
      return value;
    } catch (...) {
      throw invalid_argument_cast();
    }
  }

  template <typename ValueType>
  void set(std::string key, const ValueType& value) noexcept {
    static_assert(std::is_same<ValueType, std::string_view>::value ||
                      std::is_same<ValueType, int>::value,
                  "Only strings and integers are supported");

    arguments_.insert({key, value});
  }

 private:
  std::unordered_map<std::string, std::variant<int, std::string_view>>
      arguments_;
};

}  // namespace eagle
#endif  // EAGLE_REQUEST_ARGUMENTS_HPP
