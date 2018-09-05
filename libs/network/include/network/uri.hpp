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
//---

#include "core/logger.hpp"
#include "network/peer.hpp"

namespace fetch {
namespace network {

class Uri
{
public:
  // Construction / Destruction
  Uri() = default;
  Uri(std::string const &uristr)
  {
    data_ = uristr;
  }
  Uri(Uri const &) = default;
  Uri(Uri &&)      = default;
  ~Uri()            = default;

  std::string GetProtocol() const;
  std::string GetRemainder() const;

  Uri &operator=(Uri const &) = default;
  Uri &operator=(Uri &&) = default;

  bool operator==(Uri const &other) const
  {
    return data_ == other.data_;
  }
  bool operator<(Uri const &other) const
  {
    return data_ < other.data_;
  }

  std::string ToString() const
  {
    return data_;
  }

  Peer AsPeer() const
  {
    if (GetProtocol() == "tcp")
    {
      auto s = GetRemainder();
      FETCH_LOG_WARN("Uri","Converting: ", data_, " -> ", s);
      return Peer(s);
    }
    throw std::invalid_argument(std::string("'") + data_ + "' is not a valid TCP uri.");
  }

  template <typename T>
  friend void Serialize(T &serializer, Uri const &x)
  {
    serializer << x.data_;
  }

  template <typename T>
  friend void Deserialize(T &serializer, Uri &x)
  {
    serializer >> x.data_;
  }

  private:
      std::string data_;
};

}
}
