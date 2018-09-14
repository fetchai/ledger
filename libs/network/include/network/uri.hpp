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
#include <functional>
#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace network {

class Uri
{
public:
  // Construction / Destruction
  Uri() = default;
  Uri(std::string const &uristr);

  Uri(Uri const &) = default;
  Uri(Uri &&)      = default;
  ~Uri()            = default;

  const char *LOGGING_NAME = "Uri";

  std::string GetProtocol() const;
  std::string GetRemainder() const;

  Uri &operator=(Uri const &) = default;
  Uri &operator=(Uri &&) = default;

  bool operator==(Uri const &other) const
  {
    return data_ == other.data_;
  }
  bool operator!=(Uri const &other) const
  {
    return data_ != other.data_;
  }
  bool operator<(Uri const &other) const
  {
    return data_ < other.data_;
  }

  std::size_t hash() const
  {
    return std::hash<std::string>{}(data_);
  }

  std::string ToString() const
  {
    return data_;
  }

  size_t length() const
  {
    return data_.length();
  }

  size_t size() const
  {
    return data_.size();
  }

  bool ParseTCP(byte_array::ByteArray &b, uint16_t &port) const;

  Peer AsPeer() const
  {
    if (GetProtocol() == "tcp")
    {
      auto s = GetRemainder();
      FETCH_LOG_DEBUG("Uri","Converting: ", data_, " -> ", s);

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

template<>
struct std::hash<fetch::network::Uri>
{
  std::size_t operator()(const fetch::network::Uri &x) const
  {
    return x.hash();
  }
};
