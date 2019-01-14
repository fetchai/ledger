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

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "math/free_functions/statistics/normal.hpp"

#include <ctime>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace p2p {
namespace bayrank {

template <typename IDENTITY>
class TrustStorageInterface
{
public:
  using Gaussian = math::statistics::Gaussian<double>;
  struct Trust
  {
    IDENTITY    peer_identity;
    Gaussian    g;
    double      score;
    std::time_t last_modified = -1;

    void update_score()
    {
      score = g.mu() - 3 * g.sigma();
    }

    bool operator>(Trust const &b) const
    {
      if (score > b.score)
      {
        return true;
      }
      if (score < b.score)
      {
        return false;
      }
      return peer_identity < b.peer_identity;
    }

    bool empty()
    {
      return last_modified == -1;
    }
  };

  using TrustStore    = std::vector<Trust>;
  using IDStore       = std::unordered_map<IDENTITY, std::size_t>;
  using Iterator      = typename std::vector<Trust>::iterator;
  using ConstIterator = typename std::vector<Trust>::const_iterator;
  using Mutex         = mutex::Mutex;

  static constexpr char const *LOGGING_NAME = "TrustStorageInterface";

  TrustStorageInterface()          = default;
  virtual ~TrustStorageInterface() = default;

  TrustStorageInterface(const TrustStorageInterface &rhs) = delete;
  TrustStorageInterface(TrustStorageInterface &&rhs)      = delete;
  TrustStorageInterface &operator=(const TrustStorageInterface &rhs) = delete;
  TrustStorageInterface &operator=(TrustStorageInterface &&rhs)             = delete;
  bool                   operator==(const TrustStorageInterface &rhs) const = delete;
  bool                   operator<(const TrustStorageInterface &rhs) const  = delete;

  virtual void AddPeer(Trust &&trust) = 0;

  virtual void Update()
  {
    this->Sort();
  }

  virtual bool IsPeerKnown(IDENTITY const &peer_ident) const
  {
    FETCH_LOCK(storage_mutex_);
    return id_store_.find(peer_ident) != id_store_.end();
  }

  virtual Iterator GetPeer(IDENTITY const &peer_ident)
  {
    FETCH_LOCK(storage_mutex_);
    auto const pos = id_store_.find(peer_ident);
    return pos == id_store_.end() ? storage_.end()
                                  : (storage_.begin() + static_cast<long>(pos->second));
  }

  virtual void Remove(IDENTITY const &peer_ident)
  {
    FETCH_LOCK(storage_mutex_);
    auto const pos = id_store_.find(peer_ident);
    if (pos != id_store_.end())
    {
      storage_.erase(storage_.begin() + static_cast<long>(pos->second));
      id_store_.clear();
      for (std::size_t i = 0; i < storage_.size(); ++i)
      {
        id_store_[storage_[i].peer_identity] = i;
      }
    }
  }

  virtual ConstIterator GetPeer(IDENTITY const &peer_ident) const
  {
    FETCH_LOCK(storage_mutex_);
    auto const pos = id_store_.find(peer_ident);
    return pos == id_store_.end() ? storage_.end()
                                  : (storage_.begin() + static_cast<long>(pos->second));
  }

  virtual ConstIterator begin() const
  {
    FETCH_LOCK(storage_mutex_);
    return storage_.begin();
  }

  virtual ConstIterator end() const
  {
    FETCH_LOCK(storage_mutex_);
    return storage_.end();
  }

  virtual std::size_t size() const
  {
    FETCH_LOCK(storage_mutex_);
    return storage_.size();
  }

  virtual std::size_t index(IDENTITY const &peer_ident) const
  {
    FETCH_LOCK(storage_mutex_);
    auto it = id_store_.find(peer_ident);
    if (it == id_store_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Peer ", peer_ident, " not found in the store!");
      return storage_.size() + 1;
    }
    return it->second;
  }

protected:
  virtual void Sort()
  {
    std::sort(storage_.begin(), storage_.end(), std::greater<Trust>());
    id_store_.clear();
    for (std::size_t pos = 0; pos < storage_.size(); ++pos)
    {
      id_store_[storage_[pos].peer_identity] = pos;
    }
  }

  inline bool IsInStoreLockLess(Trust const &trust, std::string store)
  {
    bool result = false;
    auto it     = id_store_.find(trust.peer_identity);
    if (it != id_store_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Peer ", ToBase64(trust.peer_identity), " already in the ",
                     store, " store!");
      Trust &current = storage_[it->second];
      if (current.last_modified < trust.last_modified)
      {
        current.g             = trust.g;
        current.last_modified = trust.last_modified;
        current.update_score();
      }
      result = true;
    }
    return result;
  }

protected:
  TrustStore storage_;
  IDStore    id_store_;

  mutable Mutex storage_mutex_{__LINE__, __FILE__};
};

}  // namespace bayrank
}  // namespace p2p
}  // namespace fetch