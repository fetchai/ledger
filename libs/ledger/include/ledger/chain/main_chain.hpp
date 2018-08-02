#pragma once

#include "core/byte_array/byte_array.hpp"
#include "core/mutex.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/consensus/proof_of_work.hpp"
#include "ledger/chain/transaction.hpp"
#include "storage/object_store.hpp"
#include "storage/resource_mapper.hpp"
#include <map>
#include <memory>
#include <set>

// Specialise hash of byte_array for unordered map/set
namespace std {

template <>
struct hash<fetch::byte_array::ByteArray>
{
  std::size_t operator()(const fetch::byte_array::ByteArray &k) const
  {
    fetch::crypto::CallableFNV hasher;

    return hasher(k);
  }
};
}  // namespace std

namespace fetch {
namespace chain {

// Main chain contains and manages block headers. No verification of headers is
// done, the structure will purely accept new headers and provide the current
// heaviest chain. The only high cost operation O(n) should be when adding
// blocks to the tail of a chain.
//
// It is not necessary to walk up the chain at any point

// Tips keep track of all chains. The root can be used to find what block the
// tip/chain is looking for if it is loose
struct Tip
{
  fetch::byte_array::ByteArray root{"0"};
  uint64_t                     total_weight;
  bool                         loose;
};

class MainChain
{
public:
  using proof_type = fetch::chain::consensus::ProofOfWork;
  using block_type = BasicBlock<proof_type, fetch::crypto::SHA256>;
  using block_hash = fetch::byte_array::ByteArray;

  // Hard code genesis
  MainChain(uint32_t minerNumber = std::numeric_limits<uint32_t>::max()) : minerNumber_{minerNumber}
  {
    block_type genesis;
    genesis.UpdateDigest();

    // Add genesis to block chain
    genesis.loose()             = false;
    blockChain_[genesis.hash()] = genesis;

    // Create tip
    auto tip              = std::make_shared<Tip>();
    tip->total_weight     = genesis.weight();
    tip->loose            = false;
    tips_[genesis.hash()] = tip;
    heaviest_             = std::make_pair(genesis.weight(), genesis.hash());

    if (minerNumber_ != std::numeric_limits<uint32_t>::max())
    {
      RecoverFromFile();
    }
    constructing_ = false;
  }

  MainChain(MainChain const &rhs) = delete;
  MainChain(MainChain &&rhs)      = delete;
  MainChain &operator=(MainChain const &rhs) = delete;
  MainChain &operator=(MainChain &&rhs) = delete;

  bool AddBlock(block_type &block)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    // First check if block already exists
    if (blockChain_.find(block.hash()) != blockChain_.end())
    {
      //      fetch::logger.Warn("Mainchain: Trying to add already seen block");
      return false;
    }
    else
    {
      fetch::logger.Info("Mainchain: Add newly found block:");
    }

    // Check whether blocks prev hash refers to a valid tip (common case)
    std::shared_ptr<Tip> tip              = std::make_shared<Tip>();
    auto                 tipRef           = tips_.find(block.body().previous_hash);
    bool                 heaviestAdvanced = false;

    if (tipRef != tips_.end())
    {
      tip = ((*tipRef).second);
      tips_.erase(tipRef);
      tip->total_weight += block.weight();
      block.loose()       = tip->loose;
      block.totalWeight() = tip->total_weight;

      fetch::logger.Info("Mainchain: Pushing block onto already existing tip:");

      fetch::logger.Info("Mainchain: ", "W=", block.weight(), " TW=", block.totalWeight());

      // Blocks need to know if their root is not genesis, for the case where
      // another block arrives and points to this one. Non loose blocks don't
      // need this.
      if (tip->loose)
      {
        block.root() = tip->root;
      }

      // Update heaviest pointer if necessary
      if (tip->loose == false && tip->total_weight > heaviest_.first)
      {
        fetch::logger.Info("Mainchain: Updating heaviest with tip");

        heaviest_.first  = tip->total_weight;
        heaviest_.second = block.hash();

        heaviestAdvanced = true;
      }
    }
    else
    {
      // We are not building on a tip, create a new tip
      fetch::logger.Info("Mainchain: Creating new tip");

      auto blockRef = blockChain_.find(block.body().previous_hash);
      tip           = std::make_shared<Tip>();

      // Tip points to existing block
      if (blockRef != blockChain_.end())
      {
        tip->total_weight   = block.weight() + (*blockRef).second.totalWeight();
        block.totalWeight() = block.weight() + (*blockRef).second.totalWeight();
        tip->loose          = (*blockRef).second.loose();

        if ((*blockRef).second.loose())
        {
          auto const &danglingRoot = (*blockRef).second.root();
          block.root()             = danglingRoot;
          tip->root                = danglingRoot;

          // Need to register ourselves as waiting
          danglingRoot_.at(danglingRoot).insert(block.hash());
        }

        if (tip->loose == false && tip->total_weight > heaviest_.first)
        {
          fetch::logger.Info("Mainchain: creating new tip that is now heaviest! (new fork)");

          heaviest_.first  = tip->total_weight;
          heaviest_.second = block.hash();
          heaviestAdvanced = true;
        }
      }
      else
      {
        // we have a block that doesn't refer to anything
        fetch::logger.Info("Mainchain: new loose block");
        tip->root         = block.body().previous_hash;
        tip->loose        = true;
        block.root()      = block.body().previous_hash;
        block.loose()     = true;
        tip->total_weight = block.weight();
        danglingRoot_[block.body().previous_hash].insert(block.hash());
      }
    }

    // Every time we add a new block there is the possibility this will go on
    // the bottom of a branch
    auto it = danglingRoot_.find(block.hash());
    if (it != danglingRoot_.end())
    {
      fetch::logger.Info("Mainchain: This block completes a dangling root!");

      // Don't want to create a new tip if we are setting a root
      tip.reset();
      std::set<block_hash> &waitingTips = (*it).second;

      fetch::logger.Info("Mainchain: Number of dangling tips: ", waitingTips.size());

      // We have seen that people are looking for this block. Update them.
      for (auto &tipHash : waitingTips)
      {
        fetch::logger.Info("Mainchain: Walking down from tip: ", ToHex(tipHash));

        tips_[tipHash]->root  = block.body().previous_hash;
        tips_[tipHash]->loose = block.loose();
        tips_[tipHash]->total_weight += block.totalWeight();

        block_hash hash = tipHash;

        // Walk down the tree from the tip updating each block to let it know
        // its root
        while (true)
        {
          auto it = blockChain_.find(hash);
          if (it == blockChain_.end())
          {
            break;
          }

          block_type &walkBlock = (*it).second;
          if (walkBlock.loose() == false || walkBlock.root() == block.body().previous_hash)
          {
            break;
          }

          walkBlock.totalWeight() = walkBlock.totalWeight() + block.totalWeight();
          walkBlock.loose()       = block.loose();
          walkBlock.root()        = block.body().previous_hash;
          hash                    = walkBlock.body().previous_hash;
        }

        // KLL: at this point we need to see if this connection has created a
        // new heaviest tip.
        auto updatedTip = tips_[tipHash];
        if (!updatedTip->loose && updatedTip->total_weight > heaviest_.first)
        {
          fetch::logger.Info("Mainchain: Updating heaviest with tip");

          heaviest_.first  = updatedTip->total_weight;
          heaviest_.second = tipHash;

          heaviestAdvanced = true;
        }
      }

      if (block.loose())
      {
        danglingRoot_[block.body().previous_hash] = waitingTips;
      }

      // We need to update the dangling root
      danglingRoot_.erase(it);
    }

    if (tip)
    {
      // May need to update references to this tip
      UpdateTipRefs(tip, block);

      tips_[block.hash()] = tip;
    }
    blockChain_[block.hash()] = block;

    if (heaviestAdvanced)
    {
      WriteToFile();
    }

    return true;
  }

  block_type const &HeaviestBlock() const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    auto const &                         block = blockChain_.at(heaviest_.second);

    return block;
  }

  uint64_t weight() const { return heaviest_.first; }

  std::size_t totalBlocks() const { return blockChain_.size(); }

  std::vector<block_type> HeaviestChain(
      uint64_t const &limit = std::numeric_limits<uint64_t>::max()) const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    std::vector<block_type> result;

    auto topBlock = blockChain_.at(heaviest_.second);

    while ((topBlock.body().block_number != 0) && (result.size() < limit))
    {
      result.push_back(topBlock);
      auto hash = topBlock.body().previous_hash;

      // Walk down
      auto it = blockChain_.find(hash);
      if (it == blockChain_.end())
      {
        fetch::logger.Info(
            "Mainchain: Failed while walking down\
            from top block to find genesis!");
        break;
      }

      topBlock = (*it).second;
    }

    result.push_back(topBlock);  // this should be genesis
    return result;
  }

  // for debugging: get all chains, and verify. First in pair is heaviest block
  std::pair<block_type, std::vector<std::vector<block_type>>> AllChain()
  {
    // To verify, walk down the blocks making sure the block numbers decrement,
    // the weights are correct etc.
    std::lock_guard<fetch::mutex::Mutex>                        lock(mutex_);
    std::pair<block_type, std::vector<std::vector<block_type>>> result;
    std::unordered_map<block_hash, block_type>                  blockChainCopy;

    for (auto &i : tips_)
    {
      block_hash              hash        = i.first;
      uint64_t                totalWeight = i.second->total_weight;
      uint64_t                blockNumber = 0;
      bool                    first       = true;
      std::vector<block_type> chain;

      if (blockChain_.find(hash) == blockChain_.end())
      {
        fetch::logger.Error("Mainchain: Tip not found in blockchain! ", ToHex(hash));
        return result;
      }

      // Walk down from this tip
      while (true)
      {
        auto it = blockChain_.find(hash);
        if (it == blockChain_.end())
        {
          break;
        }

        block_type &walkBlock = (*it).second;

        totalWeight -= walkBlock.weight();

        if (first)
        {
          first       = false;
          blockNumber = walkBlock.body().block_number;
        }
        else
        {
          if (blockNumber != walkBlock.body().block_number + 1)
          {
            fetch::logger.Error("Blocks not sequential when walking down chain ", ToHex(hash));
            fetch::logger.Info("Prev: ", blockNumber);
            fetch::logger.Info("current: ", walkBlock.body().block_number);
            return result;
          }
          blockNumber = walkBlock.body().block_number;
        }
        hash = walkBlock.body().previous_hash;
        chain.push_back(walkBlock);
        blockChainCopy[walkBlock.hash()] = walkBlock;
      }
      result.second.push_back(chain);
    }

    // check that we have seen all blocks
    if (blockChainCopy.size() != blockChain_.size())
    {
      fetch::logger.Error(
          "Mainchain: blocks reachable from tips differ from blocks\
          in the blockchain. Tips: ",
          blockChainCopy.size(), " blockchain: ", blockChain_.size());
    }

    result.first = blockChain_.at(heaviest_.second);
    return result;
  }

  void reset()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    blockChain_.clear();
    tips_.clear();
    danglingRoot_.clear();

    // recreate genesis
    block_type genesis;
    genesis.UpdateDigest();

    // Add genesis to block chain
    genesis.loose()             = false;
    blockChain_[genesis.hash()] = genesis;

    // Create tip
    auto tip              = std::make_shared<Tip>();
    tip->total_weight     = genesis.weight();
    tip->loose            = false;
    tips_[genesis.hash()] = std::move(tip);
    heaviest_             = std::make_pair(genesis.weight(), genesis.hash());
  }

  bool Get(block_hash hash, block_type &block) const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    auto it = blockChain_.find(hash);

    if (it != blockChain_.end())
    {
      block = (*it).second;
      return true;
    }

    return false;
  }

private:
  // Long term storage and backup
  fetch::storage::ObjectStore<block_type> blockStore_;
  const uint32_t                          blockConfirmation_ = 10;
  const uint32_t                          minerNumber_;
  bool                                    constructing_ = true;

  std::unordered_map<block_hash, block_type>           blockChain_;    // all blocks are here
  std::unordered_map<block_hash, std::shared_ptr<Tip>> tips_;          // Keep track of the tips
  std::unordered_map<block_hash, std::set<block_hash>> danglingRoot_;  // Waiting (loose) tips
  std::pair<uint64_t, block_hash>                      heaviest_;      // Heaviest block/tip
  mutable fetch::mutex::Mutex                          mutex_;

  void RecoverFromFile()
  {
    std::ostringstream fileMain;
    std::ostringstream fileIndex;

    fileMain << "chain_" << minerNumber_ << ".db";
    fileIndex << "chainIndex_" << minerNumber_ << ".db";

    blockStore_.Load(fileMain.str(), fileIndex.str());

    block_type block;
    if (blockStore_.Get(storage::ResourceID("head"), block))
    {
      block.UpdateDigest();
      AddBlock(block);
    }

    while (blockStore_.Get(storage::ResourceID(block.body().previous_hash), block))
    {
      block.UpdateDigest();
      AddBlock(block);
    }
  }

  void WriteToFile()
  {
    if (constructing_ || minerNumber_ == std::numeric_limits<uint32_t>::max()) return;

    // Add confirmed blocks to file
    block_type block  = blockChain_.at(heaviest_.second);
    bool       failed = false;

    for (std::size_t i = 0; i < blockConfirmation_; ++i)
    {
      if (!GetPrev(block))
      {
        failed = true;
        break;
      }
    }

    if (!failed)
    {
      blockStore_.Set(storage::ResourceID("head"), block);
      blockStore_.Set(storage::ResourceID(block.hash()), block);
    }

    // Walk down the file to check we have an unbroken chain
    while (GetPrev(block) && !blockStore_.Has(storage::ResourceID(block.hash())))
    {
      blockStore_.Set(storage::ResourceID(block.hash()), block);
    }
  }

  inline bool GetPrev(block_type &block)
  {
    auto it = blockChain_.find(block.body().previous_hash);

    if (it == blockChain_.end())
    {
      return false;
    }

    block = (*it).second;
    return true;
  }

  // Here we have advanced a tip, but it is still loose. This means the
  // reference to this tip in danglingRoot_ will need to be updated
  void inline UpdateTipRefs(const std::shared_ptr<Tip> &tip, const block_type &block)
  {
    if (tip->loose)
    {
      auto const &tipRoot = tip->root;

      std::set<block_hash> &tipsWaitingOnRoot = danglingRoot_.at(tipRoot);

      // Set of 'tips', search for our prev tip
      auto it = tipsWaitingOnRoot.find(block.body().previous_hash);

      if (it == tipsWaitingOnRoot.end())
      {
        fetch::logger.Info("Failed to find tip in dangling root!");
        return;
      }

      tipsWaitingOnRoot.erase(it);
      tipsWaitingOnRoot.insert(block.hash());
    }
  }
};

}  // namespace chain
}  // namespace fetch
