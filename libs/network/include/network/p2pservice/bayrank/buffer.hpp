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
#include "network/p2pservice/bayrank/good_place.hpp"
#include "network/p2pservice/bayrank/bad_place.hpp"

#include <vector>
#include <unordered_map>

namespace fetch {
namespace p2p {

template <typename IDENTITY>
class TrustBuffer  : public TrustStorageInterface<IDENTITY>
{
protected:
  using TrustStorageInterface<IDENTITY>::storage_mutex_;
  using TrustStorageInterface<IDENTITY>::storage_;
  using TrustStorageInterface<IDENTITY>::id_store_;
  using TrustStorageInterface<IDENTITY>::SCORE_THRESHOLD;
  using TrustStorageInterface<IDENTITY>::SIGMA_THRESHOLD;

public:
  using Trust    = typename TrustStorageInterface<IDENTITY>::Trust;
  using Gaussian = typename TrustStorageInterface<IDENTITY>::Gaussian;

  static constexpr char const *LOGGING_NAME = "TrustBuffer";

  TrustBuffer(GoodPlace<IDENTITY>& good_place, BadPlace<IDENTITY>& bad_place) : good_place_(good_place), bad_place_(bad_place)
  {
  }
  ~TrustBuffer() = default;

  TrustBuffer(const TrustBuffer &rhs)            = delete;
  TrustBuffer(TrustBuffer &&rhs)                 = delete;
  TrustBuffer &operator=(const TrustBuffer &rhs) = delete;
  TrustBuffer &operator=(TrustBuffer &&rhs)      = delete;

  void NewPeer(IDENTITY const &peer_ident)
  {
    FETCH_LOCK(storage_mutex_);
    if (storage_.size()>=MAX_SIZE || id_store_.find(peer_ident)!=id_store_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Buffer full, can't add peer: ", ToBase64(peer_ident));
      return;
    }
    Trust new_record{peer_ident, Gaussian::ClassicForm(mu_new_peer, sigma_new_peer), 0, GetCurrentTime()};
    new_record.update_score();

    id_store_[peer_ident] = storage_.size();
    storage_.push_back(std::move(new_record));
    Sort();
  }

  int MovePeerIfEligible(IDENTITY const &peer_ident)
  {
    FETCH_LOCK(storage_mutex_);
    auto pos_it = id_store_.find(peer_ident);
    if (pos_it==id_store_.end())
    {
      return -1;
    }
    auto &peer = storage_[pos_it->second];
    FETCH_LOG_WARN(LOGGING_NAME, "(AB): ", ToBase64(peer_ident), " mu=", peer.g.mu(), ", sigma=", peer.g.sigma(), ", score=", peer.score);
    if (peer.g.sigma()>=SIGMA_THRESHOLD)
    {
      return 0;
    }
    auto pos   = static_cast<long>(pos_it->second);
    if (peer.score>SCORE_THRESHOLD)
    {
      good_place_.AddPeer(std::move(peer));
      storage_.erase(storage_.begin()+pos);
      id_store_.erase(peer_ident);
      FETCH_LOG_INFO(LOGGING_NAME,  "(AB): ", ToBase64(peer_ident), " moved to good place!");
      Sort();
      return 1;
    }
    else
    {
      bad_place_.AddPeer(std::move(peer));
      storage_.erase(storage_.begin()+pos);
      id_store_.erase(peer_ident);
      FETCH_LOG_INFO(LOGGING_NAME,  "(AB): ", ToBase64(peer_ident), " moved to good bad place!");
      Sort();
      return 2;
    }
  }

  bool AddPeer(Trust &&trust) override
  {
    return false;
  }

private:
  static inline time_t GetCurrentTime()
  {
    return std::time(nullptr);
  }
  void Sort()
  {
    std::sort(storage_.begin(), storage_.end(), std::greater<Trust>());
    for (std::size_t pos = 0; pos < storage_.size(); ++pos)
    {
      id_store_[storage_[pos].peer_identity] = pos;
    }
  }

  std::size_t const MAX_SIZE    = 1000;

  //new peer
  double const mu_new_peer      = 100.;
  double const sigma_new_peer   = 100./6.;

  //good and bad place references
  GoodPlace<IDENTITY>& good_place_;
  BadPlace<IDENTITY>&  bad_place_;
};

}  // namespace p2p
}  // namespace fetch