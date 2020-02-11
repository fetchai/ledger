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

#include "logging/logging.hpp"
#include <iostream>
#include <string>

class Uri
{
public:
  static constexpr char const *LOGGING_NAME = "Uri";

  explicit Uri(const std::string &s)
  {
    this->s = s;
    parse(s);
  }

  Uri() = default;

  Uri(const Uri &other)
  {
    copy(other);
  }
  Uri &operator=(const Uri &other)
  {
    copy(other);
    return *this;
  }

  virtual ~Uri() = default;

  std::string s;
  std::string proto;
  std::string host;
  std::string path;
  uint32_t    port{0};
  bool        valid{false};

  void diagnostic() const
  {
    FETCH_LOG_INFO(LOGGING_NAME, "valid=", valid, " proto=\"", proto, "\" host=\"", host,
                   "\" port=", port, " path=\"", path, "\"");
  }

  std::string GetSocketAddress() const
  {
    return proto + "://" + host + ":" + std::to_string(port);
  }

  std::string ToString() const
  {
    return GetSocketAddress() + "/" + path;
  }

protected:
  void parse(const std::string &s);

  void copy(const Uri &other)
  {
    s     = other.s;
    valid = other.valid;
    proto = other.proto;
    host  = other.host;
    port  = other.port;
    path  = other.path;
  }

private:
  bool operator==(const Uri &other) = delete;  // const { return compare(other)==0; }
  bool operator<(const Uri &other)  = delete;  // const { return compare(other)==-1; }
};
