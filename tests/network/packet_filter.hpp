#ifndef PACKET_FILTER_HPP
#define PACKET_FILTER_HPP

// This file hold the last seen 'packets' and filters

namespace fetch
{
namespace network_test
{

template <typename T, std::size_t size>
class PacketFilter
{
public:
  PacketFilter()
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      history_[i] = T();
    }
  }

  PacketFilter(PacketFilter &rhs)            = delete;
  PacketFilter(PacketFilter &&rhs)           = delete;
  PacketFilter operator=(PacketFilter& rhs)  = delete;
  PacketFilter operator=(PacketFilter&& rhs) = delete;

  template<typename A>
  bool Add(A&& rhs)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    for(auto &i : history_)
    {
      if(i == rhs)
      {
        return false;
      }
    }

    history_[index_++] = std::forward<A>(rhs);
    index_ = index_ % history_.size();

    return true;
  }

  void reset()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    for (std::size_t i = 0; i < history_.size(); ++i)
    {
      history_[i] = T();
    }
  }

private:
  std::size_t         index_ = 0;
  std::array<T, size> history_;
  fetch::mutex::Mutex mutex_;
};

}
}
#endif
