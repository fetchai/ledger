#ifndef MAIN_CHAIN_HPP
#define MAIN_CHAIN_HPP

#include "core/byte_array/referenced_byte_array.hpp"
#include "core/mutex.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/block.hpp"
#include "crypto/fnv.hpp"
#include<map>
#include<set>

// Specialise hash of byte_array
namespace std {

  template <>
  struct hash<fetch::byte_array::ByteArray>
  {
    std::size_t operator()(const fetch::byte_array::ByteArray& k) const
    {
      fetch::crypto::CallableFNV hasher;

      return hasher(k);
    }
  };
}

namespace fetch
{
namespace chain
{

struct Tip
{
  fetch::byte_array::ByteArray    root;
  uint64_t                        total_weight;
  bool                            loose;
};

class MainChain
{
 public:
  typedef fetch::chain::consensus::ProofOfWork          proof_type;
  typedef BasicBlock<proof_type, fetch::crypto::SHA256> block_type;
  typedef fetch::byte_array::ByteArray                  block_hash;

  MainChain(block_type &genesis)
  {
    // Add genesis to block chain
    genesis.loose()             = false;
    blockChain_[genesis.hash()] = genesis;

    // Create tip
    auto tip = std::shared_ptr<Tip>(new Tip);
    tip->total_weight     = genesis.weight();
    tip->loose            = false;
    tips_[genesis.hash()] = std::move(tip); // TODO: (`HUT`) : try
    heaviest_             = std::make_pair(genesis.weight(), genesis.hash());
  }

  MainChain(MainChain const &rhs)            = delete;
  MainChain(MainChain &&rhs)                 = delete;
  MainChain &operator=(MainChain const &rhs) = delete;
  MainChain &operator=(MainChain&& rhs)      = delete;

  void UpdateTips(block_type const &genesis)
  {
  }

  bool AddBlock(block_type &block)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    // First check if block already exists
    if(blockChain_.find(block.hash()) != blockChain_.end())
    {
      return false;
    }

    // Check whether blocks prev hash refers to a valid tip (common case)
    std::shared_ptr<Tip> tip;
    auto tipRef = tips_.find(block.body().previous_hash);

    if(tipRef != tips_.end())
    {
      // Advance that tip and put block into blockchain
      tip = std::move((*tipRef).second);
      tips_.erase(tipRef);
      block.loose() = tip->loose;
      tip->total_weight += block.weight();
      block.totalWeight() = tip->total_weight;

      // Blocks need to know if their root is not genesis
      if(tip->loose)
      {
        block.root() = ((*tipRef).second)->root;
      }

      // Update heaviest pointer if necessary
      if(tip->loose == false && tip->total_weight > heaviest_.first)
      {
        heaviest_.first = tip->total_weight;
        heaviest_.second = block.hash();
      }
    }
    else
    {
      // We are not building on a tip, create a new tip
      auto blockRef = blockChain_.find(block.body().previous_hash);
      tip = std::shared_ptr<Tip>(new Tip);

      // Tip points to existing block
      if(blockRef != blockChain_.end())
      {
        tip->total_weight     = block.weight() + (*blockRef).second.weight();
        block.totalWeight()   = block.weight() + (*blockRef).second.weight();
        tip->loose            = (*blockRef).second.loose();

        if((*blockRef).second.loose())
        {
          block.root()   = (*blockRef).second.root();
        }

        if(tip->loose == false && tip->total_weight > heaviest_.first)
        {
          heaviest_.first = tip->total_weight;
          heaviest_.second = block.hash();
        }
      }
      else
      {
        // we have a block that doesn't refer to anything
        tip->root           = block.body().previous_hash;
        tip->loose          = true;
        block.root()        = block.body().previous_hash;
        block.loose()       = true;
        danglingRoot_[block.body().previous_hash].insert(block.hash());
      }
    }

    // Every time we add a new block there is the possibility this will connect two or more trees
    auto it = danglingRoot_.find(block.hash());
    if(it != danglingRoot_.end())
    {
      // Don't want to create a new tip if we are connecting a tree
      tip.reset();

      // We have seen that people are looking for this block. Update them.
      for(auto &tipHash : (*it).second)
      {
        // TODO: (`HUT`) : in case of main chain, delete memory mgmt
        tips_[tipHash]->root = block.body().previous_hash;
        tips_[tipHash]->loose = block.loose();

        block_hash hash = tipHash;

        // Walk down the tree from the tip updating each block to let it know its root
        while(true)
        {
          auto it = blockChain_.find(hash);
          if(it == blockChain_.end())
          {
            break;
          }

          block_type &walkBlock = (*it).second;
          if(walkBlock.root() == block.hash())
          {
            break;
          }

          walkBlock.root()        = block.hash();
          walkBlock.totalWeight() = walkBlock.totalWeight() + block.totalWeight();
          hash                    = block.body().previous_hash;
        }
      }
    }

    if(tip)
    {
      tips_[block.hash()] = std::move(tip);
    }
    blockChain_[block.hash()] = block;

    return true;
  }

  block_type const &HeaviestBlock() const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    return blockChain_.at(heaviest_.second);
  }

  uint64_t weight() const
  {
    return heaviest_.first;
  }

  std::size_t totalBlocks() const
  {
    return blockChain_.size();
  }

  // TODO: (`HUT`) : callbacks

private:
  std::unordered_map<block_hash, block_type>           blockChain_;
  std::unordered_map<block_hash, std::shared_ptr<Tip>> tips_;
  std::unordered_map<block_hash, std::set<block_hash>> danglingRoot_;
  std::pair<uint64_t, block_hash>                      heaviest_;
  mutable fetch::mutex::Mutex                          mutex_;
};

}
}


#endif
