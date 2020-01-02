#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "http/authentication_level.hpp"
#include "http/header.hpp"
#include "http/method.hpp"
#include "http/query.hpp"
#include "json/document.hpp"
#include "logging/logging.hpp"
#include "network/fetch_asio.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace fetch {
namespace http {

class HTTPRequest
{
public:
  using Clock     = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;
  using Duration  = Clock::duration;

  static constexpr char const *LOGGING_NAME = "HTTPRequest";

  HTTPRequest() = default;

  bool ParseBody(asio::streambuf &buffer);
  bool ParseHeader(asio::streambuf &buffer, std::size_t end);

  Method const &method() const
  {
    return method_;
  }

  byte_array::ConstByteArray const &uri() const
  {
    return uri_;
  }

  byte_array::ConstByteArray const &protocol() const
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

  std::size_t header_length() const
  {
    return header_data_.size();
  }

  std::size_t content_length() const
  {
    return content_length_;
  }

  byte_array::ConstByteArray body() const
  {
    return body_data_;
  }

  json::JSONDocument JSON() const
  {
    return json::JSONDocument(body());
  }

  void SetMethod(Method method)
  {
    method_ = method;
  }

  void SetURI(byte_array::ConstByteArray const &uri)
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

  void SetProcessed()
  {
    processed_ = Clock::now();
  }

  double GetDuration() const
  {
    auto const duration = processed_ - created_;
    return static_cast<double>(duration.count() * Clock::period::num) /
           static_cast<double>(Clock::period::den);
  }

  void AddAuthentication(byte_array::ConstByteArray const &auth_method, uint32_t level)
  {
    if (!auth_method_.empty())
    {
      auth_method_.Append(", ");
    }

    auth_method_.Append(auth_method);
    auth_level_ |= level;
  }

  void SetAuthentication(byte_array::ConstByteArray const &auth_method, uint32_t level)
  {
    auth_method_ = auth_method;
    auth_level_  = level;
  }

  uint32_t authentication_level() const
  {
    return auth_level_;
  }

private:
  bool ParseStartLine(byte_array::ByteArray &line);

  std::string originating_address_{};
  uint16_t    originating_port_{0};

  byte_array::ByteArray header_data_;
  byte_array::ByteArray body_data_;

  Header   header_;
  QuerySet query_;

  Method                     method_{Method::GET};
  byte_array::ConstByteArray full_uri_;
  byte_array::ConstByteArray uri_;
  byte_array::ConstByteArray protocol_;

  bool is_valid_ = true;

  std::size_t content_length_ = 0;

  /// @name Metadata
  /// @{
  Timepoint created_{Clock::now()};
  Timepoint processed_{};
  /// @}

  /// Authenticated
  /// @{
  byte_array::ByteArray auth_method_{};
  uint32_t              auth_level_{AuthenticationLevel::DEFAULT_LEVEL};
  /// }
};
}  // namespace http
}  // namespace fetch
