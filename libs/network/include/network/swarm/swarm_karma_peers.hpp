#pragma once

#include "swarm_karma_peer.hpp"
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace swarm {

class SwarmKarmaPeers
{
public:
  using peers_list_type = std::vector<SwarmKarmaPeer>;
  using mutex_type = std::recursive_mutex;
  using lock_type = std::lock_guard<mutex_type>;

  SwarmKarmaPeers(std::string ident) : ident_(std::move(ident)) {}

  virtual ~SwarmKarmaPeers() {}

  bool empty() const { return peers.empty(); }

  template <class KEY>
  peers_list_type::iterator Find(const KEY &key)
  {
    lock_type mlock(mutex_);
    return std::find(peers.begin(), peers.end(), key);
  }

  template <class KEY>
  peers_list_type::const_iterator Find(const KEY &key) const
  {
    lock_type mlock(mutex_);
    return std::find(peers.begin(), peers.end(), key);
  }

  template <class KEY>
  void AddKarma(const KEY &key, double change)
  {
    lock_type                 mlock(mutex_);
    peers_list_type::iterator it = Find(key);
    if (it != peers.end())
    {
      it->AddKarma(change);
    }
  }

  template <class KEY>
  bool Has(const KEY &key)
  {
    lock_type                 mlock(mutex_);
    peers_list_type::iterator it = Find(key);
    return it != peers.end();
  }

  template <class KEY>
  double GetKarma(const KEY &key)
  {
    lock_type                 mlock(mutex_);
    peers_list_type::iterator it = Find(key);
    if (it != peers.end())
    {
      return it->GetCurrentKarma();
    }
    return 0.0;
  }

  void Age() const
  {
    lock_type mlock(mutex_);
    for (auto &peer : peers)
    {
      peer.Age();
    }
  }

  void Sort() const
  {
    lock_type mlock(mutex_);
    std::sort(peers.begin(), peers.end());
  }

  std::list<SwarmKarmaPeer> GetBestPeers(uint32_t n, double minKarma = 0.0) const
  {
    lock_type                 mlock(mutex_);
    std::list<SwarmKarmaPeer> results;
    Age();
    Sort();

    for (auto &peer : peers)
    {
      if (results.size() == n)
      {
        break;
      }
      if (peer.GetCurrentKarma() < minKarma)
      {
        break;
      }
      results.push_back(peer);
    }

    return results;
  }

  void AddOrUpdate(const SwarmPeerLocation peer, double karma)
  {
    lock_type                 mlock(mutex_);
    peers_list_type::iterator it = Find(peer);
    if (it == peers.end())
    {
      peers.push_back(SwarmKarmaPeer(peer, karma));
    }
    else
    {
      it->AddKarma(karma);
    }
  }

  void AddOrUpdate(const std::string &host, double karma)
  {
    lock_type                 mlock(mutex_);
    peers_list_type::iterator it = Find(host);
    if (it == peers.end())
    {
      peers.push_back(SwarmKarmaPeer(host, karma));
    }
    else
    {
      it->AddKarma(karma);
    }
  }

  SwarmKarmaPeer GetNthKarmicPeer(uint32_t n) const
  {
    lock_type mlock(mutex_);
    Age();
    Sort();

    uint32_t i = 0;
    for (auto peer : peers)
    {
      if (i == n)
      {
        return peer;
      }
      i++;
    }
    return peers.back();
  }

  SwarmKarmaPeers(SwarmKarmaPeers &rhs)  = delete;
  SwarmKarmaPeers(SwarmKarmaPeers &&rhs) = delete;
  SwarmKarmaPeers operator=(SwarmKarmaPeers &rhs) = delete;
  SwarmKarmaPeers operator=(SwarmKarmaPeers &&rhs) = delete;

protected:
  mutable peers_list_type peers;
  std::string             ident_;
  mutable mutex_type      mutex_;
};

}  // namespace swarm
}  // namespace fetch
