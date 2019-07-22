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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/group_definitions.hpp"
#include "crypto/fnv.hpp"
#include "network/peer.hpp"

#include <cassert>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>

namespace fetch {
namespace network {

class Uri
{
public:
  enum class Scheme
  {
    Unknown = 0,
    Tcp,
    Muddle
  };

  static constexpr char const *LOGGING_NAME = "Uri";

  using ConstByteArray = byte_array::ConstByteArray;

  // Construction / Destruction
  Uri() = default;
  explicit Uri(Peer const &peer);
  explicit Uri(ConstByteArray const &uri);
  Uri(Uri const &)     = default;
  Uri(Uri &&) noexcept = default;
  ~Uri()               = default;

  bool Parse(ConstByteArray const &uri);

  /// @name Basic Accessors
  /// @{
  ConstByteArray const &uri() const;
  Scheme                scheme() const;
  ConstByteArray const &authority() const;
  /// @}

  /// @name Type based Accessors
  /// @{
  Peer const &          AsPeer() const;
  ConstByteArray const &AsIdentity() const;
  /// @}

  // Operators
  Uri &operator=(Uri const &) = default;
  Uri &operator=(Uri &&) = default;
  bool operator==(Uri const &other) const;
  bool operator!=(Uri const &other) const;

  std::string ToString() const;

  template <typename T, typename D>
  friend struct serializers::MapSerializer;

  static Uri  FromIdentity(ConstByteArray const &identity);
  static bool IsUri(const std::string &possible_uri);

  bool IsDirectlyConnectable() const;

private:
  ConstByteArray uri_;
  Scheme         scheme_{Scheme::Unknown};
  ConstByteArray authority_;
  Peer           tcp_;
};

inline Uri::ConstByteArray const &Uri::uri() const
{
  return uri_;
}

inline Uri::Scheme Uri::scheme() const
{
  return scheme_;
}

inline Uri::ConstByteArray const &Uri::authority() const
{
  return authority_;
}

inline Peer const &Uri::AsPeer() const
{
  assert(scheme_ == Scheme::Tcp);
  return tcp_;
}

inline Uri::ConstByteArray const &Uri::AsIdentity() const
{
  assert(scheme_ == Scheme::Muddle);
  return authority_;
}

inline bool Uri::operator==(Uri const &other) const
{
  return uri_ == other.uri_;
}

inline bool Uri::operator!=(Uri const &other) const
{
  return !(*this == other);
}

inline bool Uri::IsDirectlyConnectable() const
{
  return scheme_ != Uri::Scheme::Muddle && scheme_ != Uri::Scheme::Unknown;
}

inline Uri Uri::FromIdentity(ConstByteArray const &identity)
{
  return Uri{"muddle://" + byte_array::ToBase64(identity)};
}

}  // namespace network

namespace serializers {

template <typename D>
struct MapSerializer<network::Uri, D>  // TODO: Use other than Mapserializder
{
public:
  using DriverType = D;
  using Type       = network::Uri;

  static const uint8_t URI = 1;

  template <typename T>
  static inline void Serialize(T &map_constructor, Type const &x)
  {
    auto map = map_constructor(1);
    map.Append(URI, x.uri_);
  }

  template <typename T>
  static inline void Deserialize(T &map, Type &x)
  {
    byte_array::ConstByteArray uri;
    uint8_t                    key;
    map.GetNextKeyPair(key, uri);  // TODO: check

    if (!x.Parse(uri))
    {
      throw std::runtime_error("Failed to deserialize uri");
    }
  }
};

}  // namespace serializers

}  // namespace fetch

template <>
struct std::hash<fetch::network::Uri> : private std::hash<fetch::byte_array::ConstByteArray>
{
  std::size_t operator()(fetch::network::Uri const &x) const
  {
    return std::hash<fetch::byte_array::ConstByteArray>::operator()(x.uri());
  }
};
