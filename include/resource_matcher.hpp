#ifndef EAGLE_RESOURCE_MATCHER_HPP
#define EAGLE_RESOURCE_MATCHER_HPP

#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace eagle {

enum class resource_descriptor_type { kstatic, kdynamic };

inline std::string_view to_string(resource_descriptor_type desc_type) {
  switch (desc_type) {
    case resource_descriptor_type::kstatic:
      return "S";
    case resource_descriptor_type::kdynamic:
      return "D";
    default:
      return "INVALID";
  }
}

enum class resource_descriptor_value_type { kstring, kinteger, knone };

inline std::string_view to_string(
    resource_descriptor_value_type desc_value_type) {
  switch (desc_value_type) {
    case resource_descriptor_value_type::kstring:
      return "string";
    case resource_descriptor_value_type::kinteger:
      return "integer";
    case resource_descriptor_value_type::knone:
      return "none";
    default:
      return "invalid";
  }
}

struct resource_descriptor {
  resource_descriptor() = default;

  resource_descriptor(resource_descriptor_type desc_type,
                      std::string_view desc_id)
      : type(desc_type),
        identifier(desc_id),
        value_type(resource_descriptor_value_type::knone) {}

  resource_descriptor(resource_descriptor_type desc_type,
                      std::string_view desc_id,
                      resource_descriptor_value_type value_type)
      : type(desc_type), identifier(desc_id), value_type(value_type) {}

  std::string path_view() const {
    std::stringstream ss;
    switch (type) {
      case resource_descriptor_type::kstatic:
        ss << identifier;
        break;
      case resource_descriptor_type::kdynamic:
        ss << "{" << to_string(value_type) << ":" << identifier << "}";
        break;
      default:
        break;
    }

    return ss.str();
  }

  std::string canonical_view() const {
    std::stringstream ss;
    ss << "(" << to_string(type) << ", '" << identifier << "'";

    if (value_type != resource_descriptor_value_type::knone) {
      ss << ", " << to_string(value_type);
    }

    ss << ")";

    return ss.str();
  }

  resource_descriptor_type type{resource_descriptor_type::kstatic};
  std::string identifier{""};
  resource_descriptor_value_type value_type{
      resource_descriptor_value_type::knone};
};

inline bool operator==(const resource_descriptor& a,
                       const resource_descriptor& b) {
  return (a.type == b.type) && (a.identifier == b.identifier) &&
         (a.value_type == b.value_type);
}

inline bool operator!=(const resource_descriptor& a,
                       const resource_descriptor& b) {
  return !(a == b);
}

class descriptor_list final {
 public:
  descriptor_list() = default;
  ~descriptor_list() = default;

  void add(resource_descriptor&& desc) {
    if (desc.type == resource_descriptor_type::kdynamic) {
      has_dynamic_ = true;
    }

    descriptors_.push_back(std::move(desc));
  }

  std::string canonical_view() const {
    std::stringstream ss;
    ss << "[";

    for (auto desc = descriptors_.cbegin(); desc != descriptors_.cend();) {
      ss << desc->canonical_view();

      desc++;
      if (desc != descriptors_.cend()) {
        ss << ", ";
      }
    }
    ss << "]";
    return ss.str();
  }

  std::string path_view() const {
    std::stringstream ss;
    ss << "/";

    for (auto desc = descriptors_.cbegin(); desc != descriptors_.cend();) {
      ss << desc->path_view();

      desc++;
      if (desc != descriptors_.cend()) {
        ss << "/";
      }
    }

    return ss.str();
  }

  size_t size() const { return descriptors_.size(); }

  const resource_descriptor& at(size_t idx) const {
    return descriptors_.at(idx);
  }

  bool has_dynamic_descriptor() const { return has_dynamic_; }

 private:
  bool has_dynamic_{false};
  std::vector<resource_descriptor> descriptors_;
};

inline bool operator==(const descriptor_list& a, const descriptor_list& b) {
  if (a.size() != b.size()) {
    return false;
  }

  for (int idx = 0; idx < a.size(); idx++) {
    if (a.at(idx) != b.at(idx)) {
      return false;
    }
  }
  return true;
}

inline bool operator!=(const descriptor_list& a, const descriptor_list& b) {
  return !(a == b);
}

class path_scanner final {
 public:
  path_scanner(const std::string_view& stream) : stream_(stream) {}

  ~path_scanner() = default;

  bool error() const { return had_error_; }

  descriptor_list scan() {
    while (!is_at_end()) {
      start_ = current_;

      auto c = advance();
      switch (c) {
        case '/':
          handle_root();
          break;
        case '{':
          handle_dynamic_token();
          break;
        case '}':
          // This is a syntax error, the last '}' is advanced in
          // handle_dynamic_token
          had_error_ = true;
          break;
        default:
          handle_static_token();
          break;
      }

      if (had_error_) {
        break;
      }
    }

    return descriptors_;
  }

 private:
  bool is_at_end() const { return current_ >= stream_.size(); }

  char advance() {
    current_++;
    return stream_.at(current_ - 1);
  }

  void add_descriptor(resource_descriptor_type desc_type,
                      std::string_view id,
                      resource_descriptor_value_type desc_value_type) {
    descriptors_.add(resource_descriptor{desc_type, id, desc_value_type});
  }

  void add_descriptor(resource_descriptor_type desc_type, std::string_view id) {
    descriptors_.add(resource_descriptor{desc_type, id});
  }

  char peek() const {
    if (is_at_end()) {
      return '\0';
    }

    return stream_.at(current_);
  }

  void handle_root() {
    if (peek() == '\0') {
      add_descriptor(resource_descriptor_type::kstatic, "");
    }
  }

  void handle_dynamic_token() {
    size_t length = 0;
    while (peek() != ':' and !is_at_end()) {
      advance();
      length++;
    }

    // Skip the ':'
    advance();

    auto desc_value_type = handle_value_type(length);

    length = 0;
    while (peek() != '}' and !is_at_end()) {
      advance();
      length++;
    }

    // Skip the closing '}'
    advance();

    auto identifier = stream_.substr(start_ + 1, length);

    add_descriptor(resource_descriptor_type::kdynamic, identifier,
                   desc_value_type);
  }

  resource_descriptor_value_type handle_value_type(size_t length) {
    auto value_type_str = stream_.substr(start_ + 1, length);
    start_ += value_type_str.size() + 1;

    if (value_type_str == "integer") {
      return resource_descriptor_value_type::kinteger;
    } else if (value_type_str == "string") {
      return resource_descriptor_value_type::kstring;
    } else {
      had_error_ = true;
      return resource_descriptor_value_type::knone;
    }
  }

  void handle_static_token() {
    size_t length = 0;
    while (peek() != '/' and !is_at_end()) {
      advance();
      length++;
    }

    auto path = stream_.substr(start_, length + 1);

    add_descriptor(resource_descriptor_type::kstatic, path);
  }

 private:
  const std::string_view& stream_;
  descriptor_list descriptors_;
  size_t start_{0};
  size_t current_{0};
  bool had_error_{false};
};

}  // namespace  eagle

#endif  // EAGLE_RESOURCE_MATCHER_HPP
