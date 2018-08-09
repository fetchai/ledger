#pragma once
#include "../tests/include/helper_functions.hpp"
#include "crypto/fnv.hpp"
#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>

// Thread safe non blocking structure used to store and verify transaction
// blocks

namespace fetch {
namespace network_benchmark {

template <typename FirstT, typename SecondT>
class TransactionList
{

  using hasher_type = crypto::CallableFNV;

public:
  TransactionList() { validArray_.fill(0); }

  TransactionList(TransactionList &rhs)  = delete;
  TransactionList(TransactionList &&rhs) = delete;
  TransactionList operator=(TransactionList &rhs) = delete;
  TransactionList operator=(TransactionList &&rhs) = delete;

  inline bool GetWriteIndex(std::size_t &index, FirstT const &hash)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    if (Contains(hash))
    {
      return false;
    }

    index = getIndex_++;
    return true;
  }

  template <typename T>
  bool Add(FirstT const &hash, T &&block)
  {
    std::size_t index{0};

    if (!GetWriteIndex(index, hash))
    {
      fetch::logger.Info("Failed to add hash", hash);
      return false;
    }

    std::cerr << "Writing new index: " << index << std::endl;

    hashArray_[index]  = hash;
    blockArray_[index] = std::forward<T>(block);
    validArray_[index] = 1;
    return true;
  }

  SecondT &Get(FirstT const &hash)
  {
    for (std::size_t i = 0; i < arrayMax_; ++i)
    {
      if (validArray_[i] == 1 && hashArray_[i] == hash)
      {
        auto &ref = blockArray_[i];
        return ref;
      }
    }
    fetch::logger.Error("Warning: block not found for hash: ", hash);
    exit(1);
    auto &ref = blockArray_[0];
    return ref;
  }

  bool Contains(FirstT const &hash) const
  {
    for (std::size_t i = 0; i < arrayMax_; ++i)
    {
      if (validArray_[i] == 1 && hashArray_[i] == hash)
      {
        return true;
      }
    }
    return false;
  }

  bool Seen(FirstT const &hash) const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(seenMutex_);
    if (seen_.find(hash) == seen_.end())
    {
      seen_.insert(hash);
      return false;
    }
    return true;
  }

  std::size_t size() const
  {
    std::size_t count = 0;
    for (std::size_t i = 0; i < arrayMax_; ++i)
    {
      if (validArray_[i])
      {
        count++;
      }
    }
    return count;
  }

  void WaitFor(std::size_t stopCondition)
  {
    auto waitTime = std::chrono::milliseconds(1);
    while (!(size() >= stopCondition))
    {
      std::this_thread::sleep_for(waitTime);
    }
  }

  //////////////////////////////////////////////
  // Below not performance-critical
  void reset()
  {
    getIndex_ = 0;
    validArray_.fill(0);
  }

  std::set<transaction_type> GetTransactions()
  {
    std::set<transaction_type> ret;

    int tempCounter = 0;

    for (std::size_t i = 0; i < arrayMax_; ++i)
    {
      if (validArray_[i])
      {
        for (auto &j : blockArray_[i])
        {
          ret.insert(j);
          tempCounter++;
        }
      }
    }

    return ret;
  }

  std::pair<uint64_t, uint64_t> TransactionsHash()
  {
    auto     trans = GetTransactions();
    uint32_t hash  = 5;

    hasher_type hashStruct;

    for (auto &i : trans)
    {
      hash = hash ^ static_cast<uint32_t>(hashStruct(i.summary().transaction_hash));
    }

    fetch::logger.Info("Hash is now::", hash);
    return std::pair<uint64_t, uint64_t>(size(), hash);
  }

private:
  const std::size_t        arrayMax_{200};
  std::array<FirstT, 200>  hashArray_;
  std::array<SecondT, 200> blockArray_;
  std::array<int, 200>     validArray_;
  std::size_t              getIndex_{0};
  fetch::mutex::Mutex      mutex_{__LINE__, __FILE__};

  fetch::mutex::Mutex seenMutex_{__LINE__, __FILE__};
  std::set<FirstT>    seen_;
};

}  // namespace network_benchmark
}  // namespace fetch
