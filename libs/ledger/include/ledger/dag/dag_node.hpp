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
#include "ledger/chain/transaction_serializer.hpp"
#include "ledger/chain/address.hpp"

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

  using ConstByteArray = byte_array::ConstByteArray;
  using Digest         = ConstByteArray;
  using Signature      = ConstByteArray;
  using DigestList     = std::vector<Digest>;
  using HasherType     = crypto::SHA256;

  DAGNode() = default;
  DAGNode(DAGNode const &rhs)            = default;
  DAGNode(DAGNode &&rhs)                 = default;
  DAGNode &operator=(DAGNode const &rhs) = default;
  DAGNode &operator=(DAGNode&& rhs)      = default;

  // Different types of DAG nodes.
  enum Types : uint64_t
  {
    GENESIS      = 1,   //< Used to identify the genesis DAG node
    WORK         = 2,   //< Indicates that work is stored in the contents
    DATA         = 3,   //< DAG contains data that can be used inside the contract.
    ARBITRARY    = 4,   //< DAG contains an arbitrary payload
    INVALID_NODE = 255  //< The node is not valid (default on construction)
  };

  /// Serialisable state-variables
  /// @{
  uint64_t         type{INVALID_NODE};  ///< type of the DAG node
  DigestList       previous;            ///< previous nodes.
  ConstByteArray   contents;            ///< payload to be deserialised.
  Digest           contract_digest;       ///< The contract which this node is associated with.
  crypto::Identity identity;            ///< identity of the creator (empty for now)

  /// Serialisable entries to verify state
  /// @{
  Digest    hash;       ///< DAG hash.
  Signature signature;  ///< creators signature (empty for now).
  /// }

  // bookkeeping
  uint64_t   oldest_epoch_referenced = std::numeric_limits<uint64_t>::max();
  uint64_t   weight             = 0;

  bool SetContents(Transaction const &tx)
  {
    ledger::TransactionSerializer ser{};
    ser.Serialize(tx);
    contents = ser.data();

    return true;
  }

  bool GetContents(Transaction &tx)
  {
    ledger::TransactionSerializer ser{contents};
    ser.Deserialize(tx);

    return true;
  }

  /**
   * @brief finalises the node by creating the hash.
   */
  void Finalise()
  {
    serializers::ByteArrayBuffer buf;

    buf << type;
    buf << previous;
    buf << contents;
    buf << contract_digest;
    buf << identity;
    buf << hash;
    buf << signature;
    buf << oldest_epoch_referenced;
    buf << weight;

    HasherType hasher;
    hasher.Reset();
    hasher.Update(buf.data());
    this->hash = hasher.Final();
  }

  bool Verify() const
  {
    if(hash.empty())
    {
      return false;
    }

    return crypto::Verifier::Verify(identity, hash, signature);
  }
};

template <typename T>
void Serialize(T &serializer, DAGNode const &node)
{
  serializer << node.type;
  serializer << node.previous;
  serializer << node.contents;
  serializer << node.contract_digest;
  serializer << node.identity;
  serializer << node.hash;
  serializer << node.signature;
  serializer << node.oldest_epoch_referenced;
  serializer << node.weight;
}

template <typename T>
void Deserialize(T &serializer, DAGNode &node)
{
  serializer >> node.type;
  serializer >> node.previous;
  serializer >> node.contents;
  serializer >> node.contract_digest;
  serializer >> node.identity;
  serializer >> node.hash;
  serializer >> node.signature;
  serializer >> node.oldest_epoch_referenced;
  serializer >> node.weight;
}

constexpr char const *DAGNodeTypeToString(uint64_t type)
{
  char const *text = "Unknown";

  switch (type)
  {
  case DAGNode::GENESIS:
    text = "Genesis";
    break;
  case DAGNode::WORK:
    text = "Work";
    break;
  case DAGNode::DATA:
    text = "Data";
    break;
  default:
    break;
  }

  return text;
}

}  // namespace ledger
}  // namespace fetch
