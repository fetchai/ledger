#pragma once
#include "http/header.hpp"
#include "http/mime_types.hpp"
#include "http/status.hpp"
#include "network/fetch_asio.hpp"
#include <ostream>
#include <utility>

namespace fetch {
namespace http {

class HTTPResponse : public std::enable_shared_from_this<HTTPResponse>
{
public:
  HTTPResponse() = default;

  explicit HTTPResponse(byte_array::ConstByteArray const &body, MimeType mime = {".html", "text/html"},
               Status status = Status::SUCCESS_OK)
    : body_(body), mime_(std::move(mime)), status_(status)
  {
    header_.Add("content-length", int64_t(body_.size()));
    header_.Add("content-type", mime_.type);
  }

  bool ParseHeader(asio::streambuf &buffer, std::size_t length);
  bool ParseBody(asio::streambuf &buffer, std::size_t length);
  bool ToStream(asio::streambuf &buffer) const;

  byte_array::ConstByteArray const &body() const { return body_; }
  Status status() const { return status_; }
  MimeType const &mime_type() const { return mime_; }
  Header const &header() const { return header_; }

  void AddHeader(byte_array::ConstByteArray const &key, byte_array::ConstByteArray const &value)
  {
    header_.Add(key, value);
  }

  void SetBody(byte_array::ConstByteArray const &body)
  {
    body_ = body;
  }

private:

  bool ParseFirstLine(char const *begin, char const *end);
  bool ParseHeaderLine(std::size_t line_idx, char const *begin, char const *end);

  byte_array::ConstByteArray body_;
  MimeType                   mime_;
  Status                     status_;
  Header                     header_;
};
}  // namespace http
}  // namespace fetch
