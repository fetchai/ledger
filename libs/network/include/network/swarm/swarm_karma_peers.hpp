#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

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
  using mutex_type      = std::recursive_mutex;
  using lock_type       = std::lock_guard<mutex_type>;

  SwarmKarmaPeers(std::string ident) : ident_(std::move(ident))
  {}

  virtual ~SwarmKarmaPeers()
  {}

  bool empty() const
  {
    return peers_.empty();
  }

  template <class KEY>
  peers_list_type::iterator Find(const KEY &key)
  {
    lock_type mlock(mutex_);
    return std::find(peers_.begin(), peers_.end(), key);
  }

  template <class KEY>
  peers_list_type::const_iterator Find(const KEY &key) const
  {
    lock_type mlock(mutex_);
    return std::find(peers_.begin(), peers_.end(), key);
  }

  template <class KEY>
  void AddKarma(const KEY &key, double change)
  {
    lock_type                 mlock(mutex_);
    peers_list_type::iterator it = Find(key);
    if (it != peers_.end())
    {
      it->AddKarma(change);
    }
  }

  template <class KEY>
  bool Has(const KEY &key)
  {
    lock_type                 mlock(mutex_);
    peers_list_type::iterator it = Find(key);
    return it != peers_.end();
  }

  template <class KEY>
  double GetKarma(const KEY &key)
  {
    lock_type                 mlock(mutex_);
    peers_list_type::iterator it = Find(key);
    if (it != peers_.end())
    {
      return it->GetCurrentKarma();
    }
    return 0.0;
  }

  void Age() const
  {
    lock_type mlock(mutex_);
    for (auto &peer : peers_)
    {
      peer.Age();
    }
  }

  void Sort() const
  {
    lock_type mlock(mutex_);
    std::sort(peers_.begin(), peers_.end());
  }

  std::list<SwarmKarmaPeer> GetBestPeers(uint32_t n, double minKarma = 0.0) const
  {
    lock_type                 mlock(mutex_);
    std::list<SwarmKarmaPeer> results;
    Age();
    Sort();

    for (auto &peer : peers_)
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
    if (it == peers_.end())
    {
      peers_.push_back(SwarmKarmaPeer(peer, karma));
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
    if (it == peers_.end())
    {
      peers_.push_back(SwarmKarmaPeer(host, karma));
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
    for (auto peer : peers_)
    {
      if (i == n)
      {
        return peer;
      }
      i++;
    }
    return peers_.back();
  }

  SwarmKarmaPeers(SwarmKarmaPeers &rhs)  = delete;
  SwarmKarmaPeers(SwarmKarmaPeers &&rhs) = delete;
  SwarmKarmaPeers operator=(SwarmKarmaPeers &rhs) = delete;
  SwarmKarmaPeers operator=(SwarmKarmaPeers &&rhs) = delete;

protected:
  mutable peers_list_type peers_;
  std::string             ident_;
  mutable mutex_type      mutex_;
};

}  // namespace swarm
}  // namespace fetch
