#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <iostream>
#include <list>
#include <string>

namespace fetch {
namespace swarm {

class SwarmPeerLocation
{
public:
  static std::list<SwarmPeerLocation> ParsePeerListString(const std::string &s)
  {
    auto   results = std::list<SwarmPeerLocation>();
    size_t p       = 0;

    while (p < s.length())
    {
      size_t np   = s.find(",", p);
      auto   subs = s.substr(p, np - p);
      if (subs.length() > 0)
      {
        results.push_back(SwarmPeerLocation(subs));
      }
      if (np == std::string::npos)
      {
        break;
      }
      p = np + 1;  // skip the bit we used AND the comma.
    }

    return results;
  }

  SwarmPeerLocation(const std::string &locn)
  {
    locn_ = locn;
  }

  SwarmPeerLocation(const SwarmPeerLocation &rhs)
  {
    locn_ = rhs.locn_;
  }

  SwarmPeerLocation(SwarmPeerLocation &&rhs)
  {
    locn_ = std::move(rhs.locn_);
  }

  SwarmPeerLocation &operator=(const SwarmPeerLocation &rhs)
  {
    locn_ = rhs.locn_;
    return *this;
  }

  SwarmPeerLocation &operator=(SwarmPeerLocation &&rhs)
  {
    locn_ = std::move(rhs.locn_);
    return *this;
  }

  bool operator==(const SwarmPeerLocation &other) const
  {
    return this->locn_ == other.locn_;
  }

  bool operator!=(const SwarmPeerLocation &other) const
  {
    return this->locn_ != other.locn_;
  }

  bool operator<(const SwarmPeerLocation &other) const
  {
    return this->locn_ < other.locn_;
  }

  virtual ~SwarmPeerLocation()
  {}

  std::string GetHost() const
  {
    size_t colon_pos = locn_.find(":");
    if (std::string::npos == colon_pos)
    {
      return std::string(locn_);
    }
    return locn_.substr(0, colon_pos);
  }

  uint16_t GetPort() const
  {
    size_t colon_pos = locn_.find(":");
    if (std::string::npos == colon_pos)
    {
      return 9001;
    }
    return uint16_t(std::stoi(locn_.substr(colon_pos + 1)));
  }

  const std::string &AsString() const
  {
    return locn_;
  }

private:
  std::string locn_;
};

}  // namespace swarm
}  // namespace fetch
