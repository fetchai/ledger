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
#include "core/logger.hpp"
#include "core/serializers/stl_types.hpp"
#include "network/p2pservice/p2p_service_defs.hpp"
#include "variant/variant.hpp"

#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

namespace fetch {

namespace network {

class Manifest
{
public:
  using Uri = network::Uri;

  struct Entry
  {
    Uri      remote_uri;     ///< The URI to connect to
    uint16_t local_port{0};  ///< The local port to bind to

    Entry() = default;
    Entry(Uri const uri);
    Entry(Uri const uri, uint16_t port);
  };

  using Variant           = variant::Variant;
  using ConstByteArray    = byte_array::ConstByteArray;
  using ServiceType       = network::ServiceType;
  using ServiceIdentifier = network::ServiceIdentifier;
  using ServiceMap        = std::unordered_map<ServiceIdentifier, Entry>;

  using const_iterator = ServiceMap::const_iterator;

  static constexpr char const *LOGGING_NAME = "Manifest";

  // Construction / Destruction
  Manifest()                      = default;
  Manifest(Manifest const &other) = default;
  Manifest(Manifest &&other)      = default;
  ~Manifest()                     = default;

  // Map iteration
  size_t         size() const;
  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  bool Parse(ConstByteArray const &text);

  void AddService(ServiceIdentifier service_id, Entry &&entry)
  {
    service_map_.emplace(service_id, std::move(entry));
  }

  void ForEach(std::function<void(ServiceIdentifier const &, Uri const &)> cb) const
  {
    for (auto &i : service_map_)
    {
      cb(i.first, i.second.remote_uri);
    }
  }

  bool HasService(ServiceIdentifier service_id) const
  {
    auto res = service_map_.find(service_id);
    return (res != service_map_.end());
  }

  Entry const &GetService(ServiceIdentifier service_id) const
  {
    auto res = service_map_.find(service_id);
    if (res == service_map_.end())
    {
      throw std::runtime_error("Unable to lookup requested service id");
    }

    return res->second;
    ;
  }

  network::Uri const &GetUri(ServiceIdentifier service_id) const
  {
    auto res = service_map_.find(service_id);
    if (res == service_map_.end())
    {
      throw std::runtime_error("Unable to lookup requested service id");
    }

    return res->second.remote_uri;
    ;
  }

  uint16_t GetLocalPort(ServiceIdentifier service_id) const
  {
    auto res = service_map_.find(service_id);
    if (res == service_map_.end())
    {
      throw std::runtime_error("Unable to lookup requested service id");
    }

    return res->second.local_port;
    ;
  }

  std::string ToString() const;

  template <typename T>
  friend void Serialize(T &serializer, Manifest const &x);

  template <typename T>
  friend void Deserialize(T &serializer, Manifest &x);

  Manifest &operator=(Manifest &&other) = default;
  Manifest &operator=(Manifest const &other) = default;

private:
  bool ExtractSection(Variant const &obj, ServiceType service, std::size_t instance = 0);

  ServiceMap service_map_;  ///< The underlying service map
};

inline size_t Manifest::size() const
{
  return service_map_.size();
}

inline Manifest::const_iterator Manifest::begin() const
{
  return service_map_.begin();
}

inline Manifest::const_iterator Manifest::end() const
{
  return service_map_.end();
}

inline Manifest::const_iterator Manifest::cbegin() const
{
  return service_map_.begin();
}

inline Manifest::const_iterator Manifest::cend() const
{
  return service_map_.end();
}

template <typename T>
void Serialize(T &serializer, Manifest const &x)
{
  serializer << x.service_map_;
}

template <typename T>
void Serialize(T &serializer, Manifest::Entry const &x)
{
  serializer << x.remote_uri << x.local_port;
}

template <typename T>
void Deserialize(T &serializer, Manifest &x)
{
  serializer >> x.service_map_;
}

template <typename T>
void Deserialize(T &serializer, Manifest::Entry &x)
{
  serializer >> x.remote_uri >> x.local_port;
}

}  // namespace network
}  // namespace fetch
