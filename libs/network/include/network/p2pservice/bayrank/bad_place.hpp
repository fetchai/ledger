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

#include "network/p2pservice/bayrank/trust_storage_interface.hpp"

#include <vector>
#include <unordered_map>
#include <algorithm>

namespace fetch {
namespace p2p {
namespace bayrank {

template <typename IDENTITY>
class BadPlace  : public TrustStorageInterface<IDENTITY>
{
protected:
  using TrustStorageInterface<IDENTITY>::storage_mutex_;
  using TrustStorageInterface<IDENTITY>::storage_;
  using TrustStorageInterface<IDENTITY>::id_store_;
  using TrustStorageInterface<IDENTITY>::SCORE_THRESHOLD;
  using TrustStorageInterface<IDENTITY>::SIGMA_THRESHOLD;

public:
  using Trust = typename TrustStorageInterface<IDENTITY>::Trust;

  static constexpr char const *LOGGING_NAME = "BadPlace";

  BadPlace()  = default;
  ~BadPlace() = default;

  BadPlace(const BadPlace &rhs)            = delete;
  BadPlace(BadPlace &&rhs)                 = delete;
  BadPlace &operator=(const BadPlace &rhs) = delete;
  BadPlace &operator=(BadPlace &&rhs)      = delete;

  bool AddPeer(Trust &&trust)  override
  {
    if (CheckCriteria(trust))
    {
      return false;
    }
    FETCH_LOCK(storage_mutex_);
    if (id_store_.find(trust.peer_identity)!=id_store_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Peer ", ToBase64(trust.peer_identity), " already in the store!");
      return false;
    }
    auto const size = storage_.size();
    if (size>=MAX_SIZE)
    {
      std::time_t oldest     = storage_[0].last_modified;
      std::size_t oldest_pos = 0;
      for(std::size_t i=1;i<size;++i)
      {
        if (oldest>storage_[i].last_modified)
        {
          oldest     = storage_[i].last_modified;
          oldest_pos = i;
        }
      }
      id_store_[trust.peer_identity] = oldest_pos;
      id_store_.erase(storage_[oldest_pos].peer_identity);
      storage_[oldest_pos] = std::move(trust);
    }
    else
    {
      id_store_[trust.peer_identity] = storage_.size();
      storage_.push_back(std::move(trust));
    }
    return true;
  }

private:
  inline bool CheckCriteria(Trust const &trust)
  {
    return trust.g.sigma()>SIGMA_THRESHOLD || trust.score>=SCORE_THRESHOLD;
  }

private:
    std::size_t const MAX_SIZE = 1000;
};

}  //namespace bayrank
}  // namespace p2p
}  // namespace fetch