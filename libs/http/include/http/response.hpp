#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "http/header.hpp"
#include "http/mime_types.hpp"
#include "http/status.hpp"
#include "network/fetch_asio.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace fetch {
namespace http {

class HTTPResponse : public std::enable_shared_from_this<HTTPResponse>
{
public:
  HTTPResponse() = default;

  explicit HTTPResponse(byte_array::ConstByteArray const &body,
                        MimeType mime = {".html", "text/html"}, Status status = Status::SUCCESS_OK)
    : body_(std::move(body))
    , mime_(std::move(mime))
    , status_(status)
  {
    header_.Add("content-length", int64_t(body_.size()));
    header_.Add("content-type", mime_.type);
  }

  bool ParseHeader(asio::streambuf &buffer, std::size_t length);
  bool ParseBody(asio::streambuf &buffer, std::size_t length);
  bool ToStream(asio::streambuf &buffer) const;

  byte_array::ConstByteArray const &body() const
  {
    return body_;
  }
  Status status() const
  {
    return status_;
  }
  MimeType const &mime_type() const
  {
    return mime_;
  }
  Header const &header() const
  {
    return header_;
  }

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
