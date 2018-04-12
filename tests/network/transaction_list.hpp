#ifndef TRANSACTION_LIST_HPP
#define TRANSACTION_LIST_HPP

namespace fetch
{
namespace network_test
{

template <typename T, std::size_t fixedSize>
class TransactionList
{

public:
  TransactionList() {}

  TransactionList(TransactionList &rhs)            = delete;
  TransactionList(TransactionList &&rhs)           = delete;
  TransactionList operator=(TransactionList& rhs)  = delete;
  TransactionList operator=(TransactionList&& rhs) = delete;

  template<typename A>
  void Add(A&& rhs)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    if(index_ < list_.size())
    {
      list_[index_++] = std::forward<A>(rhs);
    }
  }

  // Not performance-critical
  std::set<T> GetTransactions()
  {
    std::set<T> ret;
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    fetch::logger.Info("Returning transactions set, index is ", index_, " for a max of ", list_.size());
    std::cerr << "Returning transactions set, index is "<< index_<< " for a max of "<< list_.size() << std::endl;

    for (std::size_t i = 0; i < index_; ++i)
    {
      auto &item = list_[i];

      for (std::size_t j = i+1;j < index_; ++j)
      {
        if(item == list_[j])
        {
          fetch::logger.Info("Found duplicate transaction at index: ", i, " and ", i);
        }
      }

      ret.insert(item);
    }

    return ret;
  }

  void reset()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    index_ = 0;
  }

  std::pair<uint64_t, uint64_t> TransactionsHash()
  {
    auto trans = GetTransactions();

    uint64_t count = 0;
    uint64_t hash  = 0;

    std::cerr << "Getting hash" << std::endl;

    for(auto &i : trans)
    {
      count++;
      hash = hash ^ i.summary().transaction_hash.hash();
    }

    std::cerr << "Returning hash" << std::endl;
    return std::pair<uint64_t, uint64_t>(count, hash);
  }

  std::size_t size() const
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    return index_;
  }

private:
  std::array<T, fixedSize>     list_;
  std::size_t                  index_ = 0;
  mutable fetch::mutex::Mutex  mutex_;
};

}
}
#endif
