#ifndef MAIN_CHAIN_HPP
#define MAIN_CHAIN_HPP

#include "core/byte_array/referenced_byte_array.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/block.hpp"
//#include "./transaction.hpp" // TODO: (`HUT`) : fix

namespace fetch
{
namespace chain
{

class MainChain
{
 public:
  typedef fetch::chain::consensus::ProofOfWork          proof_type;
  typedef BasicBlock<proof_type, fetch::crypto::SHA256> block_type;

  bool AddBlock(block_type const &block)
  {
    blockChain.push_back(block);
  }

private:
  std::vector<block_type>     blockChain;
  mutable fetch::mutex::Mutex mutex_(__LINE__, __FILE__);
};

template <typename T>
inline void Serialize(T &serializer, MainChain const &b)
{
}

template <typename T>
inline void Deserialize(T &serializer, MainChain &b)
{
}

}
}
#endif
