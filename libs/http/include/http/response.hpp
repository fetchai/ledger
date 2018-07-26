#pragma once
#include "http/header.hpp"
#include "http/mime_types.hpp"
#include "http/status.hpp"
#include "network/fetch_asio.hpp"
#include <ostream>

namespace fetch {
namespace http {

class HTTPResponse : public std::enable_shared_from_this<HTTPResponse>
{
public:
  HTTPResponse(byte_array::ConstByteArray body,
               MimeType const &           mime   = {".html", "text/html"},
               Status const &             status = status_code::SUCCESS_OK)
      : body_(body), mime_(mime), status_(status)
  {
    header_.Add("content-length", int64_t(body_.size()));
    header_.Add("content-type", mime_.type);
  }

  static void WriteToBuffer(HTTPResponse &res, asio::streambuf &buffer)
  {
    LOG_STACK_TRACE_POINT;

    std::ostream out(&buffer);

    out << "HTTP/1.1 " << res.status_.code << "\r\n";

    if (!res.header_.Has("content-length"))
    {
      res.header_.Add("content-length", res.body_.size());
    }

    for (auto &field : res.header_)
    {
      out << field.first << ": " << field.second << "\r\n";
    }
    out << "\r\n";
    out.write(const_cast<const byte_array::ConstByteArray &>(res.body_)
                  .char_pointer(),
              static_cast<int>(res.body_.size()));
  }

  byte_array::ConstByteArray const &body() const { return body_; }

  Status const &status() const { return status_; }

  Status &status() { return status_; }

  MimeType const &mime_type() const { return mime_; }

  MimeType &mime_type() { return mime_; }

  Header const &header() const { return header_; }

  Header &header() { return header_; }

private:
  byte_array::ConstByteArray body_;
  MimeType                   mime_;
  Status                     status_;
  Header                     header_;
};
}  // namespace http
}  // namespace fetch

