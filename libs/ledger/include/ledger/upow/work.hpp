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
  using BigUnsigned     = math::BigUnsigned;

  /// Serialisable
  /// @{
  uint64_t block_number;
  BigUnsigned nonce;
  ScoreType score = std::numeric_limits<ScoreType>::max(); 
  /// }

  // Used internally after deserialisation
  // This information is already stored in the DAG and hence
  // we don't want to store it again.
  ContractName contract_name;
  Identity miner;

  BigUnsigned operator()()
  {
    crypto::SHA256 hasher;
    hasher.Reset();

    hasher.Update(contract_name);
    hasher.Update(miner);
    hasher.Update(nonce);

    Digest digest = hasher.Final();
    hasher.Reset();
    hasher.Update(digest);
    return hasher.Final();
  }  

  bool operator<(Work const &other) const 
  {
    return score < other.score;
  }
  
  bool operator==(Work const &other) const 
  {
    return (score == other.score) &&
           (nonce == other.nonce) &&
           (contract_name == other.contract_name) &&
           (miner == other.miner);
  }

  bool operator!=(Work const &other) const 
  {
    return !(*this == other);
  }
};

template<typename T>
void Serialize(T &serializer, Work const &work)
{
  serializer << work.block_number << work.nonce << work.score;
}

template<typename T>
void Deserialize(T &serializer, Work &work)
{
  serializer >> work.block_number >> work.nonce >> work.score;
}

}
}