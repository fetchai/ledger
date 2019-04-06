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

#include "core/json/document.hpp"
#include "core/logger.hpp"

namespace fetch {
namespace network_benchmark {

class Endpoint
{
public:
  Endpoint()
  {}

  Endpoint(const std::string &IP, const int TCPPort)
    : IP_{IP}
    , TCPPort_{uint16_t(TCPPort)}
  {}
  Endpoint(const std::string &IP, const uint16_t TCPPort)
    : IP_{IP}
    , TCPPort_{TCPPort}
  {}

  Endpoint(const json::JSONDocument &jsonDoc)
  {
    LOG_STACK_TRACE_POINT;

    IP_ = jsonDoc["IP"].As<std::string>();

    // TODO(issue 24): fix after this parsing works
    if (jsonDoc["TCPPort"].Is<uint16_t>())
    {
      TCPPort_ = jsonDoc["TCPPort"].As<uint16_t>();
    }
    else if (jsonDoc["TCPPort"].Is<float>())
    {
      float value = jsonDoc["TCPPort"].As<float>();
      TCPPort_    = uint16_t(value);
    }
    else
    {
      TCPPort_ = 0;
    }

    std::cout << "jsondoc: " << IP_ << ":" << TCPPort_ << std::endl;
  }

  bool operator<(const Endpoint &rhs) const
  {
    return (TCPPort_ < rhs.TCPPort()) || (IP_ < rhs.IP());
  }

  bool equals(const Endpoint &rhs) const
  {
    return (TCPPort_ == rhs.TCPPort()) && (IP_ == rhs.IP());
  }

  variant::Variant variant() const
  {
    LOG_STACK_TRACE_POINT;
    variant::Variant result = variant::Variant::Object();
    result["IP"]            = IP_;
    result["TCPPort"]       = TCPPort_;
    std::cout << "variant: " << IP_ << ":" << TCPPort_ << std::endl;
    return result;
  }

  const std::string &IP() const
  {
    return IP_;
  }
  std::string &IP()
  {
    return IP_;
  }
  const uint16_t &TCPPort() const
  {
    return TCPPort_;
  }
  uint16_t &TCPPort()
  {
    return TCPPort_;
  }

private:
  std::string IP_;
  uint16_t    TCPPort_ = 0;
};

}  // namespace network_benchmark
}  // namespace fetch
