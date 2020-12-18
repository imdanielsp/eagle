#ifndef EAGLE_PARAMS_HPP
#define EAGLE_PARAMS_HPP

#include <exception>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>

#include <boost/format.hpp>

namespace eagle {

class param_not_found : public std::exception {
 public:
  param_not_found(std::string_view key) : key_(key.data(), key.size()) {}

  const char* what() const noexcept override {
    output_ = key_ + " is not in the parameters list";
    return output_.c_str();
  }

 private:
  mutable std::string output_;
  std::string key_;
};

class invalid_param_cast : public std::exception {};

class params final {
 public:
  params() = default;
  ~params() = default;

  template <typename ValueType>
  const ValueType& get(std::string key) const {
    static_assert(std::is_same<ValueType, std::string_view>::value ||
                      std::is_same<ValueType, int>::value,
                  "Only strings and integers are supported");
    auto itr = params_.find(key);
    if (itr == params_.end()) {
      throw param_not_found(key);
    }

    try {
      const auto& value = std::get<ValueType>(itr->second);
      return value;
    } catch (...) {
      throw invalid_param_cast();
    }
  }

  template <typename ValueType>
  void set(std::string key, const ValueType& value) noexcept {
    static_assert(std::is_same<ValueType, std::string_view>::value ||
                      std::is_same<ValueType, int>::value,
                  "Only strings and integers are supported");

    params_.insert({key, value});
  }

 private:
  std::unordered_map<std::string, std::variant<int, std::string_view>> params_;
};

}  // namespace eagle
#endif  // EAGLE_PARAMS_HPP
