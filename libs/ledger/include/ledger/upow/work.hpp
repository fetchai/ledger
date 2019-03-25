#pragma once
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/sha256.hpp"
#include "crypto/fnv.hpp"

#include <limits>

namespace fetch
{
namespace consensus
{

struct Work 
{
  using Identity        = byte_array::ConstByteArray;
  using ContractName    = byte_array::ConstByteArray;
  using WorkId          = byte_array::ConstByteArray;
  using Digest          = byte_array::ConstByteArray;
  using ScoreType       = double; // TODO: Change to fixed point
  
  /// Serialisable
  /// @{
  int64_t nonce;
  ScoreType score = std::numeric_limits<ScoreType>::max(); 
  /// }

  /// Used internally after deserialisation
  ContractName contract_name;
  Identity miner;

  int64_t operator()()
  {
    crypto::SHA256 hasher;
    hasher.Reset();

    hasher.Update(contract_name);
    hasher.Update(miner);
    hasher.Update(nonce);

    Digest digest = hasher.Final();
    hasher.Reset();
    hasher.Update(digest);
    digest = hasher.Final();

    crypto::FNV fnv;
    fnv.Reset();
    fnv.Update(digest);

    return fnv.Final<int64_t>();
  }  

  bool operator<(Work const &other) const 
  {
    return score < other.score;
  }
  
};

template<typename T>
void Serialize(T &serializer, Work const &work)
{
  serializer << work.miner << work.nonce << work.score;
}

template<typename T>
void Deserialize(T &serializer, Work &work)
{
  serializer >> work.miner >> work.nonce >> work.score;
}

}
}