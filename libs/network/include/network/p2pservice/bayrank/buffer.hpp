#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "network/generics/future_timepoint.hpp"
#include "network/p2pservice/bayrank/bad_place.hpp"
#include "network/p2pservice/bayrank/good_place.hpp"
#include "network/p2pservice/bayrank/trust_storage_interface.hpp"

#include <random>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace p2p {
namespace bayrank {

template <typename IDENTITY>
class TrustBuffer : public TrustStorageInterface<IDENTITY>
{
protected:
  using TrustStorageInterface<IDENTITY>::storage_mutex_;
  using TrustStorageInterface<IDENTITY>::storage_;
  using TrustStorageInterface<IDENTITY>::id_store_;

public:
  using Trust    = typename TrustStorageInterface<IDENTITY>::Trust;
  using Gaussian = typename TrustStorageInterface<IDENTITY>::Gaussian;

  static constexpr char const *LOGGING_NAME = "TrustBuffer";

  TrustBuffer()
    : random_engine_(std::random_device()())
  {}
  ~TrustBuffer() = default;

  TrustBuffer(const TrustBuffer &rhs) = delete;
  TrustBuffer(TrustBuffer &&rhs)      = delete;
  TrustBuffer &operator=(const TrustBuffer &rhs) = delete;
  TrustBuffer &operator=(TrustBuffer &&rhs) = delete;

  void NewPeer(IDENTITY const &peer_ident, Gaussian const &new_peer)
  {
    FETCH_LOCK(storage_mutex_);
    if (id_store_.find(peer_ident) != id_store_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Peer already in buffer: ", ToBase64(peer_ident));
      return;
    }
    if (storage_.size() >= MAX_SIZE)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Buffer full (peer: ", ToBase64(peer_ident),
                     "), triggering cleanup!");
      Cleanup();
    }
    Trust new_record{peer_ident, new_peer, 0, GetCurrentTime()};
    new_record.update_score();

    id_store_[peer_ident] = storage_.size();
    storage_.push_back(std::move(new_record));
    this->Sort();
  }

  void AddPeer(Trust &&trust) override
  {
    FETCH_LOCK(storage_mutex_);
    if (this->IsInStoreLockLess(trust, "buffer"))
    {
      return;
    }
    auto const size = storage_.size();
    if (size >= MAX_SIZE)
    {
      if (storage_[size - 1].score <= trust.score)
      {
        id_store_[trust.peer_identity] = size - 1;
        id_store_.erase(storage_[size - 1].peer_identity);
        storage_[size - 1] = std::move(trust);
      }
    }
    else
    {
      id_store_[trust.peer_identity] = storage_.size();
      storage_.push_back(std::move(trust));
    }
    this->Sort();
  }

private:
  static inline time_t GetCurrentTime()
  {
    return std::time(nullptr);
  }

  void Cleanup()
  {
    std::vector<double> w(storage_.size());
    std::size_t         i = 0;
    for (auto it = storage_.begin(); it != storage_.end(); ++it)
    {
      // this is extra safety, not possible
      if (it->score <= 0)
      {
        id_store_.erase(it->peer_identity);
        storage_.erase(it);
        return;
      }
      w[i] = sqrt(static_cast<double>(GetCurrentTime() - it->last_modified) / 60. / it->score);
    }
    std::discrete_distribution<std::size_t> dist(w.begin(), w.end());
    auto r_it = storage_.begin() + static_cast<long>(dist(random_engine_));
    id_store_.erase(r_it->peer_identity);
    storage_.erase(r_it);
  }

  static constexpr std::size_t const MAX_SIZE = 1000;
  std::mt19937      random_engine_;
};

}  // namespace bayrank
}  // namespace p2p
}  // namespace fetch
