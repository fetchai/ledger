#pragma once

#include <memory>
#include <mutex>
#include <unordered_set>
#include <string>
#include <functional>

class SearchPeerStore
{
public:
  static constexpr char const *LOGGING_NAME = "SearchPeerStore";

  using Mutex = std::mutex;
  using Lock  = std::lock_guard<Mutex>;
  using ForBody = std::function<void(const std::string&)>;

  SearchPeerStore()                                        = default;
  virtual ~SearchPeerStore()                               = default;
  SearchPeerStore(const SearchPeerStore &other)            = delete;
  SearchPeerStore &operator=(const SearchPeerStore &other) = delete;

  bool operator==(const SearchPeerStore &other) = delete;
  bool operator<(const SearchPeerStore &other)  = delete;


  void AddPeer(const std::string& peer)
  {
    Lock lock(mutex_);
    store_.insert(peer);
  }

  void ForAllPeer(ForBody func)
  {
    Lock lock(mutex_);
    for(const auto& peer : store_)
    {
      func(peer);
    }
  }
  
protected:
private:
  Mutex mutex_;
  std::unordered_set<std::string> store_;
};
