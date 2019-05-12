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
#include "core/mutex.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/fnv.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"

#include <deque>
#include <limits>
#include <list>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace ledger {

struct DAGNode
{
  using HasherType     = crypto::SHA256;
  using ConstByteArray = byte_array::ConstByteArray;
  using Digest         = ConstByteArray;
  using Signature      = ConstByteArray;
  using DigestList     = std::vector<Digest>;
  using DigestSet      = std::unordered_set<Digest>;

  static constexpr uint64_t INVALID_TIMESTAMP = std::numeric_limits<uint64_t>::max();
  static constexpr uint64_t GENESIS_TIME      = std::numeric_limits<uint64_t>::max() - 1;

  // Different types of DAG nodes.
  enum
  {
    GENESIS      = 1,   //< Used to identify the genesis DAG node
    WORK         = 2,   //< Indicates that work is stored in the contents
    DATA         = 3,   //< DAG contains data that can be used inside the contract.
    INVALID_NODE = 255  //< The node is not valid (default on construction)
  };

  /// Properties derived from DAG and chain
  /// @{
  uint64_t timestamp{
      INVALID_TIMESTAMP};  ///< timestamp to that keeps the time since last validated block.
  DigestSet next;          ///< nodes referencing this node
  /// }

  /// Serialisable state-variables
  /// @{
  uint64_t         type{INVALID_NODE};  ///< type of the DAG node
  DigestList       previous;            ///< previous nodes.
  ConstByteArray   contents;            ///< payload to be deserialised.
  ConstByteArray   contract_name;       ///< The contract which this node is associated with.
  crypto::Identity identity;            ///< identity of the creator.
  /// }

  /// Serialisable entries to verify state
  /// @{
  Digest    hash;       ///< DAG hash.
  Signature signature;  ///< creators signature.
  /// }

  /**
   * @brief tests whether a node is valid or not.
   */
  operator bool() const
  {
    return type != INVALID_NODE;
  }

  template <typename T>
  bool SetObject(T const &obj)
  {
    serializers::ByteArrayBuffer serializer;
    serializer << obj;
    contents = serializer.data();

    return true;
  }

  template <typename T>
  bool GetObject(T &obj)
  {
    serializers::ByteArrayBuffer serializer(contents);
    serializer >> obj;

    return true;
  }

  bool operator==(DAGNode const &other) const
  {
    return (timestamp == other.timestamp) && (type == other.type) && (previous == other.previous) &&
           (contents == other.contents) && (contract_name == other.contract_name) &&
           (identity == other.identity) && (hash == other.hash) && (signature == other.signature);
  }

  /**
   * @brief finalises the node by creating the hash.
   */
  void Finalise()
  {
    serializers::ByteArrayBuffer buf;
    buf << type << previous << contents;
    buf << contract_name;
    buf << identity;

    HasherType hasher;
    hasher.Reset();
    hasher.Update(buf.data());
    this->hash = hasher.Final();
  }
};

template <typename T>
void Serialize(T &serializer, DAGNode const &node)
{
  serializer << node.type << node.previous << node.contents << node.contract_name << node.identity
             << node.hash << node.signature;
}

template <typename T>
void Deserialize(T &serializer, DAGNode &node)
{
  serializer >> node.type >> node.previous >> node.contents >> node.contract_name >>
      node.identity >> node.hash >> node.signature;
}

}  // namespace ledger
}  // namespace fetch
