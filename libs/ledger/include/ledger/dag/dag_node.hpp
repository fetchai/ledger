#pragma once

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/sha256.hpp"
#include "crypto/fnv.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/mutex.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/identity.hpp"


#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <list>
#include <limits>
#include <queue>
#include <deque>

namespace fetch {
namespace ledger {

struct DAGNode
{
  using HasherType = crypto::SHA256;
  using ConstByteArray = byte_array::ConstByteArray;
  using Digest = ConstByteArray;
  using Signature = ConstByteArray;
  using DigestList = std::vector<Digest>;

  static constexpr uint64_t INVALID_TIMESTAMP = std::numeric_limits<uint64_t>::max();
  static constexpr uint64_t GENESIS_TIME = std::numeric_limits<uint64_t>::max() - 1;

  // Different types of DAG nodes.
  enum
  {
    GENESIS = 1,
    WORK    = 2,
    DATA    = 3
  };

  uint64_t          timestamp{INVALID_TIMESTAMP};    ///< timestamp to that keeps the time since last validated block.
  uint64_t          type{WORK};                      ///< type of the DAG node
  DigestList        previous;                        ///< previous nodes.
  ConstByteArray    contents;                        ///< payload to be deserialised.  
  ConstByteArray    contract_name;                   ///< The contract which this node is associated with.
  crypto::Identity  identity;                        ///< identity of the creator.
  Digest            hash;                            ///< DAG hash.
  Signature         signature;                       ///< creators signature.

  template< typename T >
  bool SetObject(T const &obj)
  {
    serializers::ByteArrayBuffer serializer;
    serializer << obj;
    contents = serializer.data();

    return true;   
  }

  template< typename T >
  bool GetObject(T &obj)
  {
    serializers::ByteArrayBuffer serializer(contents);
    serializer >> obj;

    return true;
  }  

  bool operator==(DAGNode const &other ) const
  {
    return (timestamp == other.timestamp) &&
          (type == other.type) &&
          (previous == other.previous) &&
          (contents == other.contents) &&
          (contract_name == other.contract_name) &&
          (identity == other.identity) &&
          (hash == other.hash) &&
          (signature == other.signature);
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

template<typename T>
void Serialize(T &serializer, DAGNode const &node)
{
  serializer << node.type << node.previous << node.contents << node.contract_name << node.identity 
             << node.hash << node.signature;
}

template<typename T>
void Deserialize(T &serializer, DAGNode &node)
{
  serializer >> node.type >> node.previous >> node.contents >> node.contract_name >> node.identity
             >> node.hash >> node.signature;
}

} // namespace ledger
} // namespace fetch
