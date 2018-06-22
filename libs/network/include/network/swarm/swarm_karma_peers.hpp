#ifndef SWARM_KARMA_PEERS__
#define SWARM_KARMA_PEERS__

#include <vector>
#include "swarm_karma_peer.hpp"
#include <iostream>
#include <string>

namespace fetch
{
namespace swarm
{

class SwarmKarmaPeers
{
public:
  typedef std::vector<SwarmKarmaPeer> PEERS;
  typedef std::recursive_mutex MUTEX_T;
  typedef std::lock_guard<MUTEX_T> LOCK_T;

  SwarmKarmaPeers(const std::string &ident): ident_(ident)
  {
  }

  virtual ~SwarmKarmaPeers()
  {
  }

  bool empty() const
  {
    return peers.empty();
  }

  template<class KEY>
  PEERS::iterator Find(const KEY &key)
  {
    LOCK_T mlock(mutex_);
    return std::find(peers.begin(), peers.end(), key);
  }

  template<class KEY>
  PEERS::const_iterator Find(const KEY &key) const
  {
    LOCK_T mlock(mutex_);
    return std::find(peers.begin(), peers.end(), key);
  }

  template<class KEY>
  void AddKarma(const KEY &key, double change)
  {
    LOCK_T mlock(mutex_);
    PEERS::iterator it = Find(key);
    if (it != peers.end())
      {
        it -> AddKarma(change);
      }
  }

  template<class KEY>
  double GetKarma(const KEY &key)
  {
    LOCK_T mlock(mutex_);
    PEERS::iterator it = Find(key);
    if (it != peers.end())
      {
        return it -> GetCurrentKarma();
      }
    return 0.0;
  }

  void Age() const
  {
    LOCK_T mlock(mutex_);
    for(auto &peer: peers)
      {
        peer.Age();
      }
  }

  void Sort() const
  {
    LOCK_T mlock(mutex_);
    std::sort(peers.begin(), peers.end());

  }

  std::list<SwarmKarmaPeer> GetBestPeers(unsigned int n, double minKarma = 0.0) const
  {
    LOCK_T mlock(mutex_);
    std::list<SwarmKarmaPeer> results;
    Age();
    Sort();

    for(auto &peer: peers)
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
    LOCK_T mlock(mutex_);
    PEERS::iterator it = Find(peer);
    if (it == peers.end())
      {
        peers.push_back(SwarmKarmaPeer(peer, karma));
      }
    else
      {
        it -> AddKarma(karma);
      }
  }

  void AddOrUpdate(const std::string &host, double karma)
  {
    LOCK_T mlock(mutex_);
    PEERS::iterator it = Find(host);
    if (it == peers.end())
      {
        peers.push_back(SwarmKarmaPeer(host, karma));
      }
    else
      {
        it -> AddKarma(karma);
      }
  }

  SwarmKarmaPeer GetNthKarmicPeer(unsigned int n) const
  {
    LOCK_T mlock(mutex_);
    Age();
    Sort();

    unsigned int i = 0;
    for(auto peer: peers)
      {
        if (i==n)
          {
            return peer;
          }
        i++;
      }
    return peers.back();
  }

  SwarmKarmaPeers(SwarmKarmaPeers &rhs)            = delete;
  SwarmKarmaPeers(SwarmKarmaPeers &&rhs)           = delete;
  SwarmKarmaPeers operator=(SwarmKarmaPeers &rhs)  = delete;
  SwarmKarmaPeers operator=(SwarmKarmaPeers &&rhs) = delete;
protected:
  mutable PEERS peers;
  std::string ident_;
  mutable MUTEX_T mutex_;
};

}
}

#endif //__SWARM_KARMA_PEERS__
