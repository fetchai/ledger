#ifndef TRANSACTION_LIST_HPP
#define TRANSACTION_LIST_HPP

namespace fetch
{
namespace network_benchmark
{

template <typename T, std::size_t fixedSize>
class TransactionList
{

typedef crypto::CallableFNV hasher_type;

public:
  TransactionList() {}
  TransactionList(TransactionList &rhs)            = delete;
  TransactionList(TransactionList &&rhs)           = delete;
  TransactionList operator=(TransactionList& rhs)  = delete;
  TransactionList operator=(TransactionList&& rhs) = delete;

  void Add(std::vector<T> &&rhs)
  {
    runningCount_ += (rhs).size();
    holdTemp_[copyIndex_++] = std::move(rhs);
  }

  void Add(std::vector<T> *rhs)
  {
    runningCount_ += (*rhs).size();
    list_[ptrIndex_++] = rhs;
  }

  // Not performance-critical
  std::set<T> GetTransactions()
  {
    std::set<T> ret;
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    for (std::size_t i = 0; i < ptrIndex_; ++i)
    {
      auto item = list_[i];
      for(auto &j : *item)
      {

        /*
        fetch::logger.Info(">>HHash is ", byte_array::ToHex(j.summary().transaction_hash));
        fetch::logger.Info(">> ggroups are ");

        for(auto &k : j.summary().groups)
        {
          fetch::logger.Info(">> ", k);
        }*/

        ret.insert(j);
      }
    }

    for (std::size_t i = 0; i < copyIndex_; ++i)
    {
      auto item = holdTemp_[i];
      for(auto &j : item)
      {
        /*
        fetch::logger.Info(">>Hash is ", byte_array::ToHex(j.summary().transaction_hash));
        fetch::logger.Info(">> groups are ");

        for(auto &k : j.summary().groups)
        {
          fetch::logger.Info(">> ", k);
        } */

        ret.insert(j);
      }
    }

    return ret;
  }

  void reset()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    ptrIndex_  = 0;
    copyIndex_ = 0;
  }

  std::pair<uint64_t, uint64_t> TransactionsHash()
  {
    auto trans = GetTransactions();

    uint64_t count    = runningCount_;
    std::size_t ptrI  = ptrIndex_;
    std::size_t copyI = copyIndex_;
    uint32_t hash     = 5;

    fetch::logger.Info("\nRunning count: ", count);
    fetch::logger.Info("\nUnique count: ", trans.size());
    fetch::logger.Info("\nptr count: ", ptrI);
    fetch::logger.Info("\nlocal count: ", copyI);

    hasher_type hashStruct;

    for(auto &i : trans)
    {
      hash = hash ^ static_cast<uint32_t>(hashStruct(i.summary().transaction_hash));
    }

    fetch::logger.Info("Hash is now::", hash);

    fetch::logger.Info("returning count of size: ", count);
    return std::pair<uint64_t, uint64_t>(count, hash);
  }

  std::size_t size() const
  {
    return runningCount_;
  }

private:
  std::array<std::vector<T> *, fixedSize> list_;
  std::array<std::vector<T>, fixedSize>   holdTemp_;
  std::atomic<std::size_t>                ptrIndex_{0};
  std::atomic<std::size_t>                copyIndex_{0};
  std::atomic<std::size_t>                runningCount_{0};
  mutable fetch::mutex::Mutex             mutex_;

};

}
}
#endif
