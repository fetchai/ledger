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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "core/json/document.hpp"
#include "http/header.hpp"
#include "http/method.hpp"
#include "http/query.hpp"
#include "http/status.hpp"
#include "network/fetch_asio.hpp"

#include <algorithm>
#include <limits>

namespace fetch {
namespace http {

class HTTPRequest
{
public:
  using byte_array_type = byte_array::ConstByteArray;

  static constexpr char const *LOGGING_NAME = "HTTPRequest";

  HTTPRequest() = default;

  bool ParseBody(asio::streambuf &buffer);
  bool ParseHeader(asio::streambuf &buffer, std::size_t const &end);

  Method const &method() const
  {
    return method_;
  }

  byte_array_type const &uri() const
  {
    return uri_;
  }

  byte_array_type const &protocol() const
  {
    return protocol_;
  }

  Header const &header() const
  {
    return header_;
  }

  bool is_valid() const
  {
    return is_valid_;
  }

  QuerySet const &query() const
  {
    return query_;
  }

  std::size_t const &header_length() const
  {
    return header_data_.size();
  }

  std::size_t const &content_length() const
  {
    return content_length_;
  }

  byte_array::ConstByteArray body() const
  {
    return body_data_;
  }

  json::JSONDocument JSON() const
  {
    LOG_STACK_TRACE_POINT;

    return json::JSONDocument(body());
  }

  void SetMethod(Method method)
  {
    method_ = method;
  };

  void SetURI(byte_array_type const &uri)
  {
    uri_ = uri;
  }

  void SetBody(byte_array::ByteArray const &body)
  {
    body_data_ = body;
  }

  void AddHeader(byte_array::ConstByteArray const &key, byte_array::ConstByteArray const &value)
  {
    header_.Add(key, value);
  }

  bool ToStream(asio::streambuf &buffer, std::string const &host, uint16_t port) const;

  void SetOriginatingAddress(std::string address, uint16_t port)
  {
    originating_address_ = std::move(address);
    originating_port_    = port;
  }

  std::string const &originating_address() const
  {
    return originating_address_;
  }

  uint16_t originating_port() const
  {
    return originating_port_;
  }

private:
  void ParseStartLine(byte_array::ByteArray &line);

  std::string originating_address_{};
  uint16_t    originating_port_{0};

  byte_array::ByteArray header_data_;
  byte_array::ByteArray body_data_;

  Header   header_;
  QuerySet query_;

  Method          method_;
  byte_array_type full_uri_;
  byte_array_type uri_;
  byte_array_type protocol_;

  bool is_valid_ = true;

  std::size_t content_length_ = 0;
};
}  // namespace http
}  // namespace fetch
