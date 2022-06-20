#ifndef EAGLE_RESPONSE_HPP
#define EAGLE_RESPONSE_HPP

#include <exception>
#include <sstream>

#include <boost/beast.hpp>

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace http = beast::http;    // from <boost/beast/http.hpp>

/// This class encapsulates an eagle response base on the Boost.Beast response
/// class implementation. The goal is to hide the implementation detaul from the
/// handler and do heavy lifting work for the user in this object. For example,
/// providing a JSON writer, setting the correct content type and more.
namespace eagle {

enum class writer_type { khtml = 0, kjson, knone };

class invalid_writer_operation final : public std::exception {
 public:
  invalid_writer_operation(const char* reason) : reason_(reason) {}

  const char* what() const noexcept override { return reason_; }

 private:
  const char* reason_;
};

class response final {
 public:
  response() = default;

  ~response() = default;

  http::status result() const { return response_.result(); }

  void result(http::status v) { response_.result(v); }

  void result(unsigned int v) { response_.result(v); }

  void prepare_response() {
    beast::ostream(response_.body()) << out_stream_.str();
    response_.content_length(response_.body().size());
  }

  const auto& buffer() const { return response_; }

  /// Writers
  enum writer_type writer_type() const { return wrt_type_; }

  std::ostream& html() {
    check_writer_none_or_throw(writer_type::khtml);

    wrt_type_ = writer_type::khtml;
    response_.set(http::field::content_type, "text/html");
    return out_stream_;
  }

  std::ostream& json() {
    check_writer_none_or_throw(writer_type::kjson);

    wrt_type_ = writer_type::kjson;
    response_.set(http::field::content_type, "application/json");
    return out_stream_;
  }

 private:
  void check_writer_none_or_throw(enum writer_type wrt_type) const {
    if (!(wrt_type_ == writer_type::knone || wrt_type == wrt_type_)) {
      throw invalid_writer_operation("Only one writer can be used per request");
    }
  }

 private:
  http::response<http::dynamic_body> response_;
  std::ostringstream out_stream_;
  enum writer_type wrt_type_ { writer_type::knone };
};

}  // namespace eagle

#endif  // EAGLE_RESPONSE_HPP
