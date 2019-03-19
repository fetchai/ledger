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
  using ContractAddress = byte_array::ConstByteArray;
  using WorkId          = byte_array::ConstByteArray;
  using Digest          = byte_array::ConstByteArray;
  using ScoreType       = double; // TODO: Change to fixed point
  
  /// 
  ContractAddress contract_address;

  Identity miner;    
  int64_t nonce;
  ScoreType score = std::numeric_limits<ScoreType>::max(); 
  
  int64_t operator()()
  {
    crypto::SHA256 hasher;
    hasher.Reset();

    hasher.Update(contract_address);
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

}
}