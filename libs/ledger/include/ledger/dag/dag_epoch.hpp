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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "crypto/sha256.hpp"

#include <map>

namespace fetch {
namespace ledger {

struct DAGEpoch
{
  using ConstByteArray = byte_array::ConstByteArray;
  using HasherType     = crypto::SHA256;

  uint64_t                 block_number;
  std::set<ConstByteArray> tips;
  std::set<ConstByteArray> open_problems;

  ConstByteArray           hash;

  // TODO(HUT): consider whether to actually transmit
  // Not transmitted, but built up and compared against the hash for validity
  // map of dag node hash to dag node
  std::set<ConstByteArray>   all_nodes;

  // Useful to pass info around
  std::set<ConstByteArray> tx_digests;

  bool Contains(ConstByteArray const &hash) const
  {
    return all_nodes.find(hash) != all_nodes.end();
  }

  void Finalise()
  {
    serializers::ByteArrayBuffer buf;
    buf << *this;

    HasherType hasher;
    hasher.Reset();
    hasher.Update(buf.data());
    this->hash = hasher.Final();
  }
};

template <typename T>
void Serialize(T &serializer, DAGEpoch const &node)
{
  serializer << node.block_number;
  serializer << node.tips;
  serializer << node.open_problems;
  serializer << node.hash;
  serializer << node.all_nodes;
}

template <typename T>
void Deserialize(T &serializer, DAGEpoch &node)
{
  serializer >> node.block_number;
  serializer >> node.tips;
  serializer >> node.open_problems;
  serializer >> node.hash;
  serializer >> node.all_nodes;
}

}  // namespace ledger
}  // namespace fetch

