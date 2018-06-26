#ifndef MAIN_CHAIN_HPP
#define MAIN_CHAIN_HPP

#include "core/byte_array/referenced_byte_array.hpp"
#include "core/mutex.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/block.hpp"

namespace fetch
{
namespace chain
{

class MainChain
{
 public:
  typedef fetch::chain::consensus::ProofOfWork          proof_type;
  typedef BasicBlock<proof_type, fetch::crypto::SHA256> block_type;

  // TODO: (`HUT`) : allow partial or full main chain reconstruction
  MainChain(block_type const &genesis) : blockChain{genesis} {}

  void AddBlock(block_type const &block)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    if(blockChain[blockChain.size()-1].header() == block.body().previous_hash)
    {
      blockChain.push_back(block);
    }
  }

  std::size_t size() const
  {
    return blockChain.size();
  }

private:
  std::vector<block_type>     blockChain;
  mutable fetch::mutex::Mutex mutex_;
};

}
}
#endif
