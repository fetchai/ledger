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

#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/mutex.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace fetch {
namespace p2p {

struct TrustModifier
{
  double delta;
  double min;
  double max;
};

class TrustModifier2
{
public:
  TrustModifier2()
  {
    this->delta = 19.73;
  }
  TrustModifier2(double delta, double min, double max)
  {
    this->delta = delta;
    this->min   = min;
    this->max   = max;
  }
  double delta, min, max;

  ~TrustModifier2()
  {}
};

using trust_modifiers_type = std::array<std::array<TrustModifier, 4>, 3>;
extern const trust_modifiers_type trust_modifiers_;

inline TrustModifier const &LookupTrustModifier(TrustSubject subject, TrustQuality quality)
{
  return trust_modifiers_.at(static_cast<std::size_t>(subject))
      .at(static_cast<std::size_t>(quality));
}

template <typename IDENTITY>
class P2PTrust : public P2PTrustInterface<IDENTITY>
{
protected:
  struct PeerTrustRating
  {
    IDENTITY peer_identity;
    double   trust;
    time_t   last_modified;

    double ComputeCurrentTrust(time_t current_time) const
    {
      double const time_delta = double(std::max(0L, last_modified + 100 - current_time)) / 300.0;
      return trust * time_delta;
    }

    void SetCurrentTrust(time_t current_time)
    {
      trust         = ComputeCurrentTrust(current_time);
      last_modified = current_time;
    }
  };

  using TrustStore   = std::vector<PeerTrustRating>;
  using RankingStore = std::unordered_map<IDENTITY, size_t>;
  using Mutex        = mutex::Mutex;
  using PeerTrusts   = typename P2PTrustInterface<IDENTITY>::PeerTrusts;

public:
  using ConstByteArray = byte_array::ConstByteArray;
  using IdentitySet    = typename P2PTrustInterface<IDENTITY>::IdentitySet;
  using PeerTrust      = typename P2PTrustInterface<IDENTITY>::PeerTrust;

  static constexpr char const *LOGGING_NAME = "Trust";

  // Construction / Destruction
  P2PTrust()                    = default;
  P2PTrust(const P2PTrust &rhs) = delete;
  P2PTrust(P2PTrust &&rhs)      = delete;
  ~P2PTrust() override          = default;

  virtual void Debug() const override
  {}

  void AddFeedback(IDENTITY const &peer_ident, TrustSubject subject, TrustQuality quality) override
  {
    AddFeedback(peer_ident, ConstByteArray{}, subject, quality);
  }

  void AddFeedback(IDENTITY const &peer_ident, ConstByteArray const & /*object_ident*/,
                   TrustSubject subject, TrustQuality quality) override
  {
    FETCH_LOCK(mutex_);

    auto ranking      = ranking_store_.find(peer_ident);
    auto current_time = GetCurrentTime();

    size_t pos;
    if (ranking == ranking_store_.end())
    {
      PeerTrustRating new_record{peer_ident, 0.0, current_time};
      pos = trust_store_.size();
      trust_store_.push_back(new_record);
    }
    else
    {
      pos = ranking->second;
    }

    TrustModifier const &update = LookupTrustModifier(subject, quality);
    auto                 trust  = trust_store_[pos].ComputeCurrentTrust(current_time);

    if ((std::isnan(update.max) || (trust < update.max)) &&
        (std::isnan(update.min) || (trust > update.min)))
    {
      trust += update.delta;
    }

    trust_store_[pos].trust         = trust;
    trust_store_[pos].last_modified = current_time;

    dirty_ = true;
    SortIfNeeded();
  }

  bool IsPeerKnown(IDENTITY const &peer_ident) const override
  {
    FETCH_LOCK(mutex_);
    return ranking_store_.find(peer_ident) != ranking_store_.end();
  }

  IdentitySet GetRandomPeers(std::size_t maximum_count, double minimum_trust) const override
  {
    IdentitySet result;
    result.reserve(maximum_count);

    std::random_device rd;
    std::mt19937       g(rd());

    {
      FETCH_LOCK(mutex_);

      for (size_t pos = 0; pos < trust_store_.size(); pos++)
      {
        if (trust_store_[pos].trust < minimum_trust)
        {
          break;
        }

        result.insert(trust_store_[pos].peer_identity);
      }
    }

    return result;
  }

  PeerTrusts GetPeersAndTrusts() const override
  {
    FETCH_LOCK(mutex_);

    PeerTrusts trust_list;

    for (std::size_t pos = 0, end = trust_store_.size(); pos < end; ++pos)
    {
      PeerTrust pt;
      pt.address = trust_store_[pos].peer_identity;
      pt.name    = std::string(byte_array::ToBase64(pt.address));
      pt.trust   = trust_store_[pos].trust;
      trust_list.push_back(pt);
    }

    return trust_list;
  }

  IdentitySet GetBestPeers(std::size_t maximum) const override
  {
    IdentitySet result;
    result.reserve(maximum);

    {
      FETCH_LOCK(mutex_);

      for (std::size_t pos = 0, end = std::min(maximum, trust_store_.size()); pos < end; ++pos)
      {
        if (trust_store_[pos].trust < 0.0)
        {
          break;
        }

        result.insert(trust_store_[pos].peer_identity);
      }
    }

    return result;
  }

  std::size_t GetRankOfPeer(IDENTITY const &peer_ident) const override
  {
    FETCH_LOCK(mutex_);

    auto const ranking_it = ranking_store_.find(peer_ident);
    if (ranking_it == ranking_store_.end())
    {
      return trust_store_.size() + 1;
    }
    else
    {
      return ranking_it->second;
    }
  }

  double GetTrustRatingOfPeer(IDENTITY const &peer_ident) const override
  {
    double ranking = 0.0;

    FETCH_LOCK(mutex_);

    auto ranking_it = ranking_store_.find(peer_ident);
    if (ranking_it != ranking_store_.end())
    {
      if (ranking_it->second < trust_store_.size())
      {
        ranking = trust_store_[ranking_it->second].ComputeCurrentTrust(GetCurrentTime());
      }
    }

    return ranking;
  }

  bool IsPeerTrusted(IDENTITY const &peer_ident) const override
  {
    return GetTrustRatingOfPeer(peer_ident) > 0.0;
  }

  // Operators
  P2PTrust operator=(const P2PTrust &rhs) = delete;
  P2PTrust operator=(P2PTrust &&rhs) = delete;

protected:
  void SortIfNeeded() const
  {
    auto const current_time = GetCurrentTime();

    if (!dirty_)
    {
      return;
    }
    dirty_ = false;

    for (size_t pos = 0; pos < trust_store_.size(); pos++)
    {
      trust_store_[pos].SetCurrentTrust(current_time);
    }

    std::sort(trust_store_.begin(), trust_store_.end(),
              [](const PeerTrustRating &a, const PeerTrustRating &b) {
                if (a.trust < b.trust)
                {
                  return true;
                }
                if (a.trust > b.trust)
                {
                  return false;
                }
                return a.peer_identity < b.peer_identity;
              });

    ranking_store_.clear();
    for (std::size_t pos = 0; pos < trust_store_.size(); ++pos)
    {
      ranking_store_[trust_store_[pos].peer_identity] = pos;
    }
  }

  static time_t GetCurrentTime()
  {
    return std::time(nullptr);
  }

private:
  mutable bool         dirty_ = false;
  mutable Mutex        mutex_{__LINE__, __FILE__};
  mutable TrustStore   trust_store_;
  mutable RankingStore ranking_store_;
};

}  // namespace p2p
}  // namespace fetch
