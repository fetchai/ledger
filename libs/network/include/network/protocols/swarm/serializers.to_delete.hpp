#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "protocols/swarm/entry_point.hpp"
#include "protocols/swarm/node_details.hpp"

namespace fetch {
namespace serializers {

template <typename T>
T &Serialize(T &serializer, protocols::EntryPoint const &data)
{
  serializer << data.host;
  serializer << data.group;
  serializer << data.port;
  serializer << data.http_port;
  serializer << data.configuration;
  return serializer;
}

template <typename T>
T &Deserialize(T &serializer, protocols::EntryPoint &data)
{
  serializer >> data.host;
  serializer >> data.group;
  serializer >> data.port;
  serializer >> data.http_port;
  serializer >> data.configuration;
  return serializer;
}

template <typename T>
T &Serialize(T &serializer, protocols::NodeDetails const &data)
{
  serializer << data.public_key << data.default_port << data.default_http_port;
  serializer << uint64_t(data.entry_points.size());
  for (auto const &e : data.entry_points)
  {
    serializer << e;
  }
  return serializer;
}

template <typename T>
T &Deserialize(T &serializer, protocols::NodeDetails &data)
{
  serializer >> data.public_key >> data.default_port >> data.default_http_port;
  uint64_t size;
  serializer >> size;
  data.entry_points.resize(size);

  for (auto &e : data.entry_points)
  {
    serializer >> e;
  }
  return serializer;
}

template <typename T>
T &Serialize(T &serializer, std::vector<protocols::NodeDetails> const &data)
{
  serializer << uint64_t(data.size());
  for (auto const &e : data)
  {
    serializer << e;
  }
  return serializer;
}

template <typename T>
T &Deserialize(T &serializer, std::vector<protocols::NodeDetails> &data)
{
  uint64_t size;
  serializer >> size;
  data.resize(size);
  for (auto &e : data)
  {
    serializer >> e;
  }
  return serializer;
}
}  // namespace serializers
}  // namespace fetch
