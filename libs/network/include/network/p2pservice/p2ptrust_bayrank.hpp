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
#include "math/free_functions/statistics/normal.hpp"
#include "network/p2pservice/bayrank/bad_place.hpp"
#include "network/p2pservice/bayrank/buffer.hpp"
#include "network/p2pservice/bayrank/good_place.hpp"
#include "network/p2pservice/bayrank/object_cache.hpp"
#include "network/p2pservice/bayrank/the_train.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <iostream>
#include <map>
#include <ostream>
#include <random>
#include <string>
#include <vector>

namespace fetch {
namespace p2p {

using TrustQualityArrayGaussian = std::array<math::statistics::Gaussian<double>, 5>;
using ReferencePlayers          = std::unordered_map<TrustSubject, TrustQualityArrayGaussian>;
extern ReferencePlayers const reference_players_;

inline math::statistics::Gaussian<double> const &LookupReferencePlayer(TrustSubject subject,
                                                                       TrustQuality quality)
{
  auto it = reference_players_.find(subject);
  if (it != reference_players_.end())
  {
    return it->second.at(static_cast<std::size_t>(quality));
  }
  throw std::runtime_error("Trust subject not in the reference players map:  " +
                           std::string(ToString(subject)));
}

std::ostream &operator<<(std::ostream &o, const fetch::p2p::TrustQuality &q);
std::ostream &operator<<(std::ostream &o, const fetch::p2p::TrustSubject &q);

template <typename IDENTITY>
class P2PTrustBayRank : public P2PTrustInterface<IDENTITY>
{
public:
  using IDStore               = std::unordered_map<IDENTITY, std::size_t>;
  using Mutex                 = mutex::Mutex;
  using ConstByteArray        = byte_array::ConstByteArray;
  using TrustStorageInterface = bayrank::TrustStorageInterface<IDENTITY>;
  using GoodPlace             = bayrank::GoodPlace<IDENTITY>;
  using BadPlace              = bayrank::BadPlace<IDENTITY>;
  using TrustBuffer           = bayrank::TrustBuffer<IDENTITY>;
  using TheTrain              = bayrank::TheTrain<IDENTITY>;
  using ObjectStore           = bayrank::ObjectCache<ConstByteArray, IDENTITY>;
  using IdentitySet           = typename P2PTrustInterface<IDENTITY>::IdentitySet;
  using PeerTrust             = typename P2PTrustInterface<IDENTITY>::PeerTrust;
  using PeerTrusts            = typename P2PTrustInterface<IDENTITY>::PeerTrusts;
  using Gaussian              = typename TrustStorageInterface::Gaussian;
  using PlaceEnum             = typename bayrank::TheTrain<IDENTITY>::PLACE;

  static constexpr char const *LOGGING_NAME = "TrustBayRank";

  // Construction / Destruction
  P2PTrustBayRank()
  {}
  P2PTrustBayRank(const P2PTrustBayRank &rhs) = delete;
  P2PTrustBayRank(P2PTrustBayRank &&rhs)      = delete;
  ~P2PTrustBayRank() override                 = default;

  void AddFeedback(IDENTITY const &peer_ident, TrustSubject subject, TrustQuality quality) override
  {
    AddFeedback(peer_ident, ConstByteArray{}, subject, quality);
  }

  void AddFeedback(IDENTITY const &peer_ident, ConstByteArray const &object_ident,
                   TrustSubject subject, TrustQuality quality) override
  {
    FETCH_LOG_INFO(LOGGING_NAME, "AddFeedback: peer: ", ToBase64(peer_ident),
                   ", subject: ", subject, ", quality: ", quality);
    FETCH_LOCK(mutex_);

    try
    {
      Gaussian const &reference_player = LookupReferencePlayer(subject, quality);

      auto id_it = id_store_.find(peer_ident);
      if (id_it == id_store_.end())
      {
        id_store_[peer_ident] = 0;
        auto g                = quality == TrustQuality::NEW_PEER
                     ? reference_player
                     : LookupReferencePlayer(subject, TrustQuality::NEW_PEER);
        buffer_.NewPeer(peer_ident, g);
        id_it = id_store_.find(peer_ident);
      }
      if (quality == TrustQuality::NEW_PEER)
      {
        return;
      }

      bool honest = quality == TrustQuality::NEW_INFORMATION || quality == TrustQuality::DUPLICATE;
      if (honest && object_ident.size() > 0)
      {
        AddObject(object_ident, peer_ident);
      }

      auto sp      = stores_[id_it->second];
      auto peer_it = sp->GetPeer(peer_ident);
      if (peer_it == sp->end())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Peer ", ToBase64(peer_ident),
                       " not found in the storage (id=", id_it->second, ")!");
        return;
      }

      updateGaussian(honest, peer_it->g, reference_player);
      peer_it->update_score();
      peer_it->last_modified = GetCurrentTime();
      sp->Update();
      FETCH_LOG_WARN(LOGGING_NAME, "(AB): ", ToBase64(peer_ident),
                     " updated: score=", peer_it->score);

      auto place = the_train_.MoveIfPossible(ToPlaceEnum(id_it->second), peer_ident);
      auto s_idx = FromPlaceEnum(place);
      if (place != PlaceEnum::UNKNOWN && s_idx != -1)
      {
        id_store_[peer_ident] = static_cast<std::size_t>(s_idx);
      }
    }
    catch (std::exception &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Exception in AddFeedback: ", e.what());
    }
  }

  void AddObjectFeedback(ConstByteArray const &object_ident, TrustSubject /*subject*/,
                         TrustQuality          quality) override
  {
    object_store_.Iterate(object_ident, [this, quality](IDENTITY const &identity) {
      AddFeedback(identity, TrustSubject ::OBJECT, quality);
    });
  }

  void AddObject(ConstByteArray const &object_ident, IDENTITY const &peer_ident) override
  {
    object_store_.Add(object_ident, peer_ident);
  }

  void RemoveObject(ConstByteArray const &object_ident) override
  {
    object_store_.Remove(object_ident);
  }

  bool IsPeerKnown(IDENTITY const &peer_ident) const override
  {
    FETCH_LOCK(mutex_);
    return id_store_.find(peer_ident) != id_store_.end();
  }

  IdentitySet GetRandomPeers(std::size_t maximum_count, double minimum_trust) const override
  {
    auto gps                  = good_place_.size();
    auto good_and_buffer_size = gps + buffer_.size();
    if (maximum_count > good_and_buffer_size)
    {
      return GetBestPeers(maximum_count);
    }

    IdentitySet result;
    result.reserve(maximum_count);

    size_t                                     max_trial = maximum_count * 1000;
    std::random_device                         rd;
    std::mt19937                               g(rd());
    std::uniform_int_distribution<std::size_t> distribution(0, good_and_buffer_size - 1);

    {
      FETCH_LOCK(mutex_);

      for (std::size_t i = 0, pos = 0, inserted_element_counter = 0; i < max_trial; ++i)
      {
        pos     = distribution(g);
        auto it = good_place_.begin();
        if (pos < gps)
        {
          it = it + static_cast<long>(pos);
          if (it == good_place_.end() || it->score < minimum_trust)
          {
            continue;
          }
        }
        else
        {
          it = buffer_.begin() + static_cast<long>(pos - gps);
          if (it == buffer_.end() || it->score < minimum_trust)
          {
            continue;
          }
        }

        result.insert(it->peer_identity);
        inserted_element_counter += 1;
        if (inserted_element_counter >= maximum_count)
        {
          break;
        }
      }
    }

    return result;
  }

  IdentitySet GetBestPeers(std::size_t maximum) const override
  {
    IdentitySet result;

    auto max = std::min(maximum, buffer_.size() + good_place_.size());
    if (max < 1)
    {
      return result;
    }
    result.reserve(max);

    {
      FETCH_LOCK(mutex_);

      std::size_t pos = 0, end = 0;
      auto        good_it = good_place_.begin();
      for (end = std::min(max, good_place_.size()); pos < end; ++pos)
      {
        result.insert((good_it + static_cast<long>(pos))->peer_identity);
      }
      if (pos < max)
      {
        auto buffer_it = buffer_.begin();
        for (end = std::min(max - pos, buffer_.size()), pos = 0; pos < end; ++pos)
        {
          auto it = buffer_it + static_cast<long>(pos);
          if (it->score < TheTrain::SCORE_THRESHOLD)
          {
            break;
          }
          result.insert(it->peer_identity);
        }
      }
    }

    return result;
  }

  std::size_t GetRankOfPeer(IDENTITY const &peer_ident) const override
  {
    FETCH_LOCK(mutex_);
    auto const id_it = id_store_.find(peer_ident);
    if (id_it == id_store_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Peer ", peer_ident, " not in the trust system!");
      return good_place_.size() + buffer_.size() - 1;
    }
    if (id_it->second == 1)
    {
      return good_place_.index(peer_ident);
    }
    if (id_it->second == 0)
    {
      return good_place_.size() + buffer_.index(peer_ident);
    }
    FETCH_LOG_WARN(LOGGING_NAME, "Peer ", peer_ident,
                   " not in good place or in buffer! Ranking of someone from the bad place doesn't "
                   "make sense!");
    return good_place_.size() + buffer_.size() - 1;
  }

  PeerTrusts GetPeersAndTrusts() const override
  {
    FETCH_LOCK(mutex_);
    PeerTrusts trust_list;

    for (std::size_t store_id = 0; store_id < stores_.size(); ++store_id)
    {
      for (std::size_t pos = 0, end = stores_[store_id]->size(); pos < end; ++pos)
      {
        PeerTrust   pt;
        auto const &p     = stores_[store_id]->begin() + static_cast<long>(pos);
        pt.address        = p->peer_identity;
        pt.name           = std::string(byte_array::ToBase64(pt.address));
        pt.trust          = p->score;
        pt.has_transacted = true;
        trust_list.push_back(pt);
      }
    }
    return trust_list;
  }

  double GetTrustRatingOfPeer(IDENTITY const &peer_ident) const override
  {
    double ranking = 0.0;

    FETCH_LOCK(mutex_);

    auto id_it = id_store_.find(peer_ident);
    if (id_it != id_store_.end())
    {
      ranking = stores_[id_it->second]->GetPeer(peer_ident)->score;
    }
    return ranking;
  }

  double GetTrustUncertaintyOfPeer(IDENTITY const &peer_ident) const override
  {
    double sigma = 0.0;

    FETCH_LOCK(mutex_);

    auto id_it = id_store_.find(peer_ident);
    if (id_it != id_store_.end())
    {
      sigma = stores_[id_it->second]->GetPeer(peer_ident)->g.sigma();
    }

    return sigma;
  }

  bool IsPeerTrusted(IDENTITY const &peer_ident) const override
  {
    FETCH_LOCK(mutex_);

    auto id_it = id_store_.find(peer_ident);
    if (id_it != id_store_.end())
    {
      if (id_it->second == 0)
      {
        return buffer_.GetPeer(peer_ident)->score > TheTrain::SCORE_THRESHOLD;
      }
      return id_it->second == 1;
    }
    return false;
  }

  double GetPeerUsefulness(IDENTITY const &peer_ident) const override
  {
    FETCH_LOCK(mutex_);
    auto   id_it = id_store_.find(peer_ident);
    double w     = 0.01;
    if (id_it != id_store_.end())
    {
      auto peer = stores_[id_it->second]->GetPeer(peer_ident);
      w = sqrt(peer->score / static_cast<double>(GetCurrentTime() - peer->last_modified) / 60.);
    }
    return w;
  }

  virtual void Debug() const override
  {
    FETCH_LOCK(mutex_);
    for (auto const &id : id_store_)
    {
      auto store = stores_[id.second];
      for (auto it = store->begin(); it != store->end(); ++it)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "trust_store_", id.second, ": ",
                       byte_array::ToBase64(it->peer_identity), " => ", it->score);
      }
    }
  }

  // Operators
  P2PTrustBayRank operator=(const P2PTrustBayRank &rhs) = delete;
  P2PTrustBayRank operator=(P2PTrustBayRank &&rhs) = delete;

protected:
  Gaussian truncate(Gaussian const &g, double BETA, double EPS)
  {
    // Calculate approximated truncated Gaussian
    double m =
        std::sqrt(2) * BETA * math::statistics::normal::quantile<double>(0, 1, (EPS + 1.) / 2.);
    double k = sqrt(g.pi());
    double r = g.tau() / k - m * k;
    double v = math::statistics::normal::pdf<double>(0, 1, r) /
               math::statistics::normal::cdf<double>(0, 1, r);
    double w = v * (v + r);

    double   new_pi  = g.pi() / (1 - w);
    double   new_tau = (g.tau() + k * v) / (1 - w);
    Gaussian t       = Gaussian(new_pi, new_tau);

    return t / g;
  }

  void updateGaussian(bool honest, Gaussian &s, Gaussian const &ref)
  {
    // Calculate new distribution for g1 assuming that g1 won with g2.
    // BETA corresponds to a measure of how difficult the game is to master.
    // DRIFT corresponds to a natural DRIFT of your score between "games".
    // EPS is a draw margin by which you must be "better" to beat your opponent.
    s *= DRIFT;
    Gaussian s_ref = ref * DRIFT;
    Gaussian h     = s * BETA;
    Gaussian h_ref = s_ref * BETA;

    if (honest)
    {
      Gaussian u = truncate(h - h_ref, BETA, EPS);
      s *= (u + h_ref) * BETA;
    }
    else
    {
      Gaussian u = truncate(h_ref - h, BETA, EPS);
      s *= (-u + h_ref) * BETA;
    }
  }

  static inline time_t GetCurrentTime()
  {
    return std::time(nullptr);
  }

protected:
  mutable Mutex mutex_{__LINE__, __FILE__};

  GoodPlace   good_place_;
  BadPlace    bad_place_;
  TrustBuffer buffer_;
  IDStore     id_store_;
  ObjectStore object_store_;
  TheTrain    the_train_{buffer_, good_place_, bad_place_};

  std::array<TrustStorageInterface *, 3> stores_{{&buffer_, &good_place_, &bad_place_}};

  static inline PlaceEnum ToPlaceEnum(std::size_t i)
  {
    PlaceEnum r;
    if (i == 0)
    {
      r = PlaceEnum::BUFFER;
    }
    else if (i == 1)
    {
      r = PlaceEnum::GOOD;
    }
    else if (i == 2)
    {
      r = PlaceEnum::BAD;
    }
    else
    {
      r = PlaceEnum::UNKNOWN;
    }
    return r;
  }

  static inline int FromPlaceEnum(PlaceEnum p)
  {
    int idx = -1;
    if (p == PlaceEnum::BUFFER)
    {
      idx = 0;
    }
    else if (p == PlaceEnum::GOOD)
    {
      idx = 1;
    }
    else if (p == PlaceEnum::BAD)
    {
      idx = 2;
    }
    return idx;
  }

  /**
   * Constants
   */
  static constexpr double const BETA  = 100 / 12.;
  static constexpr double const DRIFT = 1 / 6.;
  static constexpr double const EPS   = 0.2;
};

}  // namespace p2p
}  // namespace fetch
