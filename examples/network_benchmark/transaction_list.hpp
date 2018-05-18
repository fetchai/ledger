#ifndef TRANSACTION_LIST_HPP
#define TRANSACTION_LIST_HPP
#include<type_traits>
#include<unordered_map>
#include<utility>
#include<set>
#include"crypto/fnv.hpp"
#include"../tests/include/helper_functions.hpp"

namespace fetch
{
namespace network_benchmark
{

template <typename FirstT, typename SecondT>
class TransactionList
{

typedef crypto::CallableFNV hasher_type;

public:
  TransactionList() {}
  TransactionList(TransactionList &rhs)            = delete;
  TransactionList(TransactionList &&rhs)           = delete;
  TransactionList operator=(TransactionList& rhs)  = delete;
  TransactionList operator=(TransactionList&& rhs) = delete;

  template <typename T>
  bool Add(T &&rhs)
  {
    std::unique_lock<fetch::mutex::Mutex> lock(mutex_);
    if (blocksMap_.find(rhs.first) != blocksMap_.end())
    {
      return false;
    }

    runningCount_++;
    if (!std::is_lvalue_reference<T>{})
    {
      blocksMap_[rhs.first] = std::move(rhs.second);
    } else {
      blocksMap_[rhs.first] = rhs.second;
    }

    lock.unlock();

    // Note: this should be fairly infrequent and so not a large performance hit
    if (runningCount_ >= stopCondition_)
    {
      fetch::logger.Info("Notifying!");
      stopConditional.notify_one();
    }

    fetch::logger.Info("Running count: ", runningCount_, " AKA ",
        blocksMap_.size(), " stop cond: ", stopCondition_);

    return true;
  }

  std::pair<FirstT, SecondT> Get(FirstT const &hash)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    auto iter = blocksMap_.find(hash);
    if (iter == blocksMap_.end())
    {
      std::cerr << "Warning: request for invalid block" << std::endl;
      return make_pair(FirstT(), SecondT());
    }
    return *iter;
  }

  bool Contains(FirstT const &hash) const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    return !(blocksMap_.find(hash) == blocksMap_.end());
  }

  std::size_t size() const
  {
    return runningCount_;
  }

  void WaitFor(std::size_t stopCondition)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    stopCondition_ = stopCondition;
    if (runningCount_ >= stopCondition_) { return; }
    stopConditional.wait(lock, [this] { return runningCount_ >= stopCondition_; });
  }

  //////////////////////////////////////////////
  // Below not performance-critical
  void reset()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    runningCount_ = 0;
    blocksMap_.erase(blocksMap_.begin(), blocksMap_.end());
  }

  std::set<transaction_type> GetTransactions()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    std::set<transaction_type> ret;

    int tempCounter = 0;

    for (auto &blocks : blocksMap_)
    {
      for (auto &transaction : blocks.second)
      {
        transaction.UpdateDigest();
        ret.insert(transaction);
        tempCounter++;
      }
    }
    return ret;
  }

  std::pair<uint64_t, uint64_t> TransactionsHash()
  {
    auto trans    = GetTransactions();
    uint32_t hash = 5;

    fetch::logger.Info("\nRunning count: ", runningCount_);

    hasher_type hashStruct;

    for (auto &i : trans)
    {
      i.UpdateDigest();
      hash = hash ^ static_cast<uint32_t>(hashStruct(i.summary().transaction_hash));
    }

    fetch::logger.Info("Hash is now::", hash);

    fetch::logger.Info("returning count of size: ", runningCount_);
    return std::pair<uint64_t, uint64_t>(runningCount_, hash);
  }

private:
  std::unordered_map<FirstT, SecondT>  blocksMap_;
  std::size_t                          runningCount_{0};
  std::size_t                          stopCondition_{1000};
  mutable fetch::mutex::Mutex          mutex_;
  mutable std::condition_variable      stopConditional;
};

}
}
#endif
