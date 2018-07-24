#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP
#include "core/assert.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "network/fetch_asio.hpp"
#include "http/header.hpp"
#include "http/method.hpp"
#include "http/query.hpp"
#include "http/status.hpp"
#include "core/json/document.hpp"

#include <algorithm>
#include <limits>

namespace fetch {
namespace http {

class HTTPRequest {
 public:
  typedef byte_array::ConstByteArray byte_array_type;

  HTTPRequest() {}

  static bool SetBody(HTTPRequest &req, asio::streambuf &buffer) {
    LOG_STACK_TRACE_POINT;

    // TODO: Handle encoding
    req.body_data_ = byte_array::ByteArray();
    req.body_data_.Resize(req.content_length());
    if (buffer.size() < req.body_data_.size()) {
      req.is_valid_ = false;
      return false;
    }

    std::istream is(&buffer);
    std::istreambuf_iterator<char> cit(is);

    for (std::size_t i = 0; i < req.body_data_.size(); ++i) {
      req.body_data_[i] = uint8_t(*cit);
      ++cit;
    }
    return true;
  }

  static bool SetHeader(HTTPRequest &req, asio::streambuf &buffer,
                        std::size_t const &end) {
    LOG_STACK_TRACE_POINT;

    req.header_data_ = byte_array::ByteArray();
    req.header_data_.Resize(end);

    std::istream is(&buffer);

    if (buffer.size() < end) {
      TODO_FAIL("trying to extract more than possible");
    }

    std::istreambuf_iterator<char> cit(is);

    std::size_t last_pos = 0, split_key_at = 0, split_val_at = 0;
    std::size_t line = 0;
    byte_array::ByteArray key, value, start_line;

    for (std::size_t i = 0; i < end; ++i) {
      char c = *cit;
      ++cit;

      req.header_data_[i] = uint8_t(c);

      switch (c) {
        case ':':
          if (split_key_at == 0) {
            split_val_at = split_key_at = i;
            ++split_val_at;
            while ((i + 1 < end) && (*cit) == ' ') {
              ++split_val_at;
              ++i;
              ++cit;
            }
          }
          break;
        case '\n':
          last_pos = i + 1;
          split_key_at = 0;
          break;
        case '\r':
          if (last_pos != i) {
            if (line > 0) {
              key =
                  req.header_data_.SubArray(last_pos, split_key_at - last_pos);

              for (std::size_t t = 0; t < key.size(); ++t) {
                char &cc = reinterpret_cast<char &>(key[t]);
                if (('A' <= cc) && (cc <= 'Z')) cc = char(cc + 'a' - 'A');
              }

              ++split_key_at;
              value = req.header_data_.SubArray(split_val_at, i - split_val_at);

              if (key == "content-length") {
                req.content_length_ = uint64_t(value.AsInt());
              }

              req.header_.Add(key, value);
            } else {
              start_line = req.header_data_.SubArray(0, i);
            }

            ++line;
          }

          break;
      }
    }

    req.ParseStartLine(start_line);

    return true;
  }

  Method const &method() const { return method_; }

  byte_array_type const &uri() const { return uri_; }

  byte_array_type const &protocol() const { return protocol_; }

  Header const &header() const { return header_; }

  bool is_valid() const { return is_valid_; }

  QuerySet const &query() const { return query_; }

  std::size_t const &header_length() const { return header_data_.size(); }

  std::size_t const &content_length() const { return content_length_; }

  byte_array::ConstByteArray body() const { return body_data_; }

  json::JSONDocument JSON() const {
    LOG_STACK_TRACE_POINT;

    return json::JSONDocument(body());
  }

 private:
  void ParseStartLine(byte_array::ByteArray &line) {
    LOG_STACK_TRACE_POINT;

    std::size_t i = 0;
    while (line[i] != ' ') {
      if (i >= line.size()) {
        is_valid_ = false;
        return;
      }

      char &cc = reinterpret_cast<char &>(line[i]);
      if (('A' <= cc) && (cc <= 'Z')) cc = char(cc + 'a' - 'A');
      ++i;
    }

    byte_array_type method = line.SubArray(0, i);
    if (method == "get") {
      method_ = Method::GET;
    } else if (method == "post") {
      method_ = Method::POST;
    } else if (method == "put") {
      method_ = Method::PUT;
    } else if (method == "patch") {
      method_ = Method::PATCH;
    } else if (method == "delete") {
      method_ = Method::DELETE;
    } else if (method == "options") {
      method_ = Method::OPTIONS;
    }

    ++i;

    std::size_t j = i;
    while (line[i] != ' ') {
      if (i >= line.size()) {
        is_valid_ = false;
        return;
      }

      ++i;
    }

    full_uri_ = line.SubArray(j, i - j);

    // Extracting URI parameters
    std::size_t k = j;
    while (k < i) {
      if (line[k] == '?') {
        break;
      }
      ++k;
    }

    uri_ = line.SubArray(j, k - j);

    std::size_t last = k + 1, equal = std::size_t(-1);
    byte_array::ByteArray key, value;

    while (k < i) {
      switch (line[k]) {
        case '=':
          equal = k;
          break;
        case '&':
          equal = std::min(k, equal);
          key = line.SubArray(last, equal - last);
          equal += (equal < k);
          value = line.SubArray(equal, k - equal);

          query_.Add(key, value);
          equal = std::size_t(-1);
          last = k + 1;
          break;
      }
      ++k;
    }

    equal = std::min(k, equal);
    key = line.SubArray(last, equal - last);
    equal += (equal < k);
    value = line.SubArray(equal, k - equal);
    query_.Add(key, value);

    while (line[i] == ' ') {
      if (i >= line.size()) {
        is_valid_ = false;
        return;
      }
      ++i;
    }
    protocol_ = line.SubArray(i, line.size() - i);
    for (std::size_t t = i; t < line.size(); ++t) {
      char &cc = reinterpret_cast<char &>(line[t]);
      if (('A' <= cc) && (cc <= 'Z')) cc = char(cc + 'a' - 'A');
    }
  }

  byte_array::ByteArray header_data_;
  byte_array::ByteArray body_data_;

  Header header_;
  QuerySet query_;

  Method method_;
  byte_array_type full_uri_;
  byte_array_type uri_;
  byte_array_type protocol_;

  bool is_valid_ = true;

  std::size_t content_length_ = 0;
};
}
}

#endif
