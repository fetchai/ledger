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

  template<typename A>
  void Add(A&& rhs)
  {
    {
      std::stringstream stream;
      stream << "locking" << std::endl;
      std::cerr << stream.str();
    }
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    {
      std::stringstream stream;
      stream << "checking size" << std::endl;
      std::cerr << stream.str();
    }

    if(index_ < list_.size())
    {
      {
        std::stringstream stream;
        stream << "index is " << index_ << std::endl;
        std::cerr << stream.str();
      }
      list_[index_++] = std::forward<A>(rhs);
      {
        std::stringstream stream;
        stream << "finishing" << std::endl;
        std::cerr << stream.str();
      }
    }
  }

  // Not performance-critical
  std::set<T> GetTransactions()
  {
    std::set<T> ret;
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    fetch::logger.Info("Returning transactions set, index is ", index_, " for a max of ", list_.size());
    {
      std::stringstream stream;
      stream << "Returning transactions set, index is "<< index_<< " for a max of "<< list_.size() << std::endl;
      std::cerr << stream.str();
    }

    for (std::size_t i = 0; i < index_; ++i)
    {
      auto &item = list_[i];


//      for (std::size_t j = i+1;j < index_; ++j)
//      {
//        if(item == list_[j])
//        {
//          fetch::logger.Info("Found duplicate transaction at index: ", i, " and ", i);
//        }
//      }

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

    {
      std::stringstream stream;
      stream << "Getting hash" << std::endl;
      std::cerr << stream.str();
    }
    hasher_type hashStruct;

    for(auto &i : trans)
    {
      count++;
      hash = hash ^ static_cast<uint64_t>(hashStruct(i.summary().transaction_hash));
    }

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
