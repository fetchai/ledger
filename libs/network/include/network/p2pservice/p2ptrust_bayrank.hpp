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
#include "core/byte_array/encoders.hpp"
#include "core/mutex.hpp"
#include "math/free_functions/statistics/normal.hpp"
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
#include <ostream>


namespace fetch {
namespace p2p {


using reference_players_type = std::array<math::statistics::Gaussian<double>, 4>;
extern const reference_players_type reference_players_;

inline math::statistics::Gaussian<double> const &LookupReferencePlayer(TrustQuality quality)
{
  return reference_players_.at(static_cast<std::size_t>(quality));
}

inline uint32_t CalculateID(uint32_t store_id, size_t position)
{
  uint32_t pos = static_cast<uint32_t>(position);
  uint32_t id  = 0;
  id |= pos << 16u;
  id |= store_id;
  return id;
}

inline uint32_t PositionFromID(uint32_t id)
{
  uint32_t pos      = 0;
  pos |= id >> 16u;
  return pos;
}

inline uint32_t StoreIDFromID(uint32_t id)
{
  uint32_t store_id = 0;
  store_id |= (id << 16u) >> 16u;
  return store_id;
}

std::ostream& operator<<(std::ostream& o, const fetch::p2p::TrustQuality& q);

template <typename IDENTITY>
class P2PTrustBayRank : public P2PTrustInterface<IDENTITY>
{
protected:
  using Gaussian          = math::statistics::Gaussian<double>;
  struct PeerTrustRating
  {
    IDENTITY peer_identity;
    Gaussian g;
    double   score;
    std::time_t last_modified;
    void     update_score()
    {
      score = g.mu() - 3 * g.sigma();
    }
  };
  using TrustStore = std::vector<PeerTrustRating>;
  using IDStore    = std::unordered_map<IDENTITY, uint32_t>;
  using RMutex     = std::recursive_mutex;
  using RLock      = std::unique_lock<RMutex>;

public:
  using ConstByteArray = byte_array::ConstByteArray;
  using IdentitySet    = typename P2PTrustInterface<IDENTITY>::IdentitySet;
  using PeerTrust      = typename P2PTrustInterface<IDENTITY>::PeerTrust;

  static constexpr char const *LOGGING_NAME = "TrustBayRank";

  // Construction / Destruction
  P2PTrustBayRank()                           = default;
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
    FETCH_LOG_INFO(LOGGING_NAME, "AddFeedback: peer: ",ToBase64(peer_ident), ", quality: ", quality);
    RLock lock(rmutex_);

    if (quality == TrustQuality::NEW_PEER)
    {
      if (buffer_trust_store_.size()>=MAX_BUFFER_TRUST_STORE_SIZE || id_store_.find(peer_ident)!=id_store_.end())
      {
        return;
      }
      PeerTrustRating new_record{peer_ident, Gaussian::ClassicForm(mu_new_peer, sigma_new_peer), 0, GetCurrentTime()};
      new_record.update_score();

      id_store_[peer_ident] = CalculateID(0, buffer_trust_store_.size());
      buffer_trust_store_.push_back(new_record);
      SortStore(0);
      return;
    }

    auto id_it = id_store_.find(peer_ident);
    if (id_it == id_store_.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Peer ", ToBase64(peer_ident), " not found in the IDStore!");
      AddFeedback(peer_ident, object_ident, subject, TrustQuality::NEW_PEER);
      id_it = id_store_.find(peer_ident);
    }
    uint32_t pos      = PositionFromID(id_it->second);
    uint32_t store_id = StoreIDFromID(id_it->second);

    Gaussian const &reference_player = LookupReferencePlayer(quality);

    auto& peer  = (*trust_stores_[store_id])[pos];
    bool honest = quality == TrustQuality::NEW_INFORMATION || quality == TrustQuality::DUPLICATE;

    updateGaussian(honest, peer.g, reference_player);
    peer.update_score();
    peer.last_modified = GetCurrentTime();

    if (store_id==0 && peer.g.sigma()<sigma_threshold_)
    {
      if (peer.score>score_threshold_)
      {
        auto gts_size = good_trust_store_.size();
        if (gts_size>=MAX_GOOD_TRUST_STORE_SIZE)
        {
          if (good_trust_store_[gts_size-1].score<=peer.score)
          {
            id_store_[peer_ident] = CalculateID(1, gts_size-1);
            id_store_.erase(good_trust_store_[gts_size-1].peer_identity);
            good_trust_store_[gts_size] = std::move(peer);
            buffer_trust_store_.erase(buffer_trust_store_.begin()+pos);
            SortStore(1);
          }
        }
        else
        {
          id_store_[peer_ident] = CalculateID(1, gts_size);
          FETCH_LOG_WARN(LOGGING_NAME, "Store_ID=", store_id);
          good_trust_store_.push_back(std::move(peer));
          buffer_trust_store_.erase(buffer_trust_store_.begin()+pos);
          SortStore(1);
        }
      }
      else
      {
        auto bts_size = bad_trust_store_.size();
        if (bts_size>=MAX_BAD_TRUST_STORE_SIZE)
        {
          std::time_t oldest = bad_trust_store_[0].last_modified;
          size_t oldest_pos  = 0;
          for(size_t i=1;i<bts_size;++i)
          {
            if (oldest>bad_trust_store_[i].last_modified)
            {
              oldest     = bad_trust_store_[i].last_modified;
              oldest_pos = i;
            }
          }
          id_store_[peer_ident] = CalculateID(2, oldest_pos);
          id_store_.erase(bad_trust_store_[oldest_pos].peer_identity);
          bad_trust_store_[oldest_pos] = std::move(peer);
          buffer_trust_store_.erase(buffer_trust_store_.begin()+pos);
        }
        else
        {
          id_store_[peer_ident] = CalculateID(2, bts_size);
          bad_trust_store_.push_back(std::move(peer));
          buffer_trust_store_.erase(buffer_trust_store_.begin()+pos);
        }
      }
    }
    SortStore(0);
  }

  bool IsPeerKnown(IDENTITY const &peer_ident) const override
  {
    RLock lock(rmutex_);
    return id_store_.find(peer_ident) != id_store_.end();
  }

  IdentitySet GetRandomPeers(std::size_t maximum_count, double minimum_trust) const override
  {
    if (maximum_count > good_trust_store_.size())
    {
      return GetBestPeers(maximum_count);
    }

    IdentitySet result;
    result.reserve(maximum_count);

    size_t                                max_trial = maximum_count * 1000;
    std::random_device                    rd;
    std::mt19937                          g(rd());
    std::uniform_int_distribution<size_t> distribution(0, good_trust_store_.size() - 1);

    {
      RLock lock(rmutex_);

      for (std::size_t i = 0, pos = 0, inserted_element_counter = 0; i < max_trial; ++i)
      {
        pos = distribution(g);
        if (good_trust_store_[pos].score < minimum_trust)
        {
          continue;
        }

        result.insert(good_trust_store_[pos].peer_identity);
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

    auto max = std::min(maximum, buffer_trust_store_.size()+good_trust_store_.size());
    if (max<1)
    {
      return result;
    }
    result.reserve(max);

    {
      RLock lock(rmutex_);

      std::size_t pos = 0, end = 0;
      for (end = std::min(max, good_trust_store_.size()); pos < end; ++pos)
      {
        result.insert(good_trust_store_[pos].peer_identity);
      }
      if (pos<max)
      {
        for(end=std::min(max-pos,buffer_trust_store_.size()),pos=0;pos<end;++pos)
        {
          if (buffer_trust_store_[pos].score<score_threshold_)
          {
            break;
          }
          result.insert(buffer_trust_store_[pos].peer_identity);
        }
      }
    }

    return result;
  }

  std::size_t GetRankOfPeer(IDENTITY const &peer_ident) const override
  {
    RLock lock(rmutex_);

    auto const id_it = id_store_.find(peer_ident);
    if (id_it == id_store_.end())
    {
      return good_trust_store_.size()+1;
    }
    else
    {
      auto store_id = StoreIDFromID(id_it->second);
      auto pos      = PositionFromID(id_it->second);
      if (store_id!=1)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "You are trying to get the ranking of a peer which isn't in the good_trust_store! Peer: ", peer_ident);
        return good_trust_store_.size()+1;
      }
      return pos;
    }
  }

  std::list<PeerTrust>  GetPeersAndTrusts() const override
  {
    RLock lock(rmutex_);
    auto trust_list = std::list<PeerTrust>();

    for(std::size_t store_id = 0; store_id<trust_stores_.size();++store_id)
    {
      for (std::size_t pos = 0, end = trust_stores_[store_id]->size(); pos < end; ++pos)
      {
        PeerTrust pt;
        auto const &p = (*trust_stores_[store_id])[pos];
        pt.address = p.peer_identity;
        pt.name = std::string(byte_array::ToBase64(pt.address));
        pt.trust = p.score;
        trust_list.push_back(pt);
      }
    }
    return trust_list;
  }

  double GetTrustRatingOfPeer(IDENTITY const &peer_ident) const override
  {
    double ranking = 0.0;

    RLock lock(rmutex_);

    auto id_it = id_store_.find(peer_ident);
    if (id_it != id_store_.end())
    {
      auto store_id = StoreIDFromID(id_it->second);
      auto pos      = PositionFromID(id_it->second);
      if (pos>=(*trust_stores_[store_id]).size())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "GetTrustRatingOfPeer pos>size! This shouldn't happen! pos=", pos, ", size=", (*trust_stores_[store_id]).size());
        return ranking;
      }
      ranking = (*trust_stores_[store_id])[pos].score;
    }
    return ranking;
  }

  double GetTrustUncertaintyOfPeer(IDENTITY const &peer_ident) const override
  {
    double sigma = 0.0;

    RLock lock(rmutex_);

    auto id_it = id_store_.find(peer_ident);
    if (id_it != id_store_.end())
    {
      auto store_id = StoreIDFromID(id_it->second);
      auto pos      = PositionFromID(id_it->second);
      if (pos>=(*trust_stores_[store_id]).size())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "GetTrustUncertaintyOfPeer pos>size! This shouldn't happen! pos=", pos, ", size=", (*trust_stores_[store_id]).size());
        return sigma;
      }
      sigma = (*trust_stores_[store_id])[pos].g.sigma();
    }

    return sigma;
  }

  bool IsPeerTrusted(IDENTITY const &peer_ident) const override
  {
    RLock lock(rmutex_);

    auto id_it = id_store_.find(peer_ident);
    if (id_it != id_store_.end())
    {
      auto store_id = StoreIDFromID(id_it->second);
      auto pos      = PositionFromID(id_it->second);
      return store_id==1 && pos<good_trust_store_.size();
    }
    return false;
  }

  // Operators
  P2PTrustBayRank operator=(const P2PTrustBayRank &rhs) = delete;
  P2PTrustBayRank operator=(P2PTrustBayRank &&rhs) = delete;

protected:
  Gaussian truncate(Gaussian const &g, double beta, double eps)
  {
    // Calculate approximated truncated Gaussian
    double m =
        std::sqrt(2) * beta * math::statistics::normal::quantile<double>(0, 1, (eps + 1.) / 2.);
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
    // beta corresponds to a measure of how difficult the game is to master.
    // drift corresponds to a natural drift of your score between "games".
    // eps is a draw margin by which you must be "better" to beat your opponent.
    s *= drift;
    Gaussian s_ref = ref * drift;
    Gaussian h     = s * beta;
    Gaussian h_ref = s_ref * beta;

    if (honest)
    {
      Gaussian u = truncate(h - h_ref, beta, eps);
      s *= (u + h_ref) * beta;
    }
    else
    {
      Gaussian u = truncate(h_ref - h, beta, eps);
      s *= (-u + h_ref) * beta;
    }
  }

  void SortStore(uint32_t store_id) const
  {
    std::sort(trust_stores_[store_id]->begin(), trust_stores_[store_id]->end(),
              [](const PeerTrustRating &a, const PeerTrustRating &b) {
                if (a.score > b.score)
                {
                  return true;
                }
                if (a.score < b.score)
                {
                  return false;
                }
                return a.peer_identity < b.peer_identity;
              });

    for (std::size_t pos = 0; pos < trust_stores_[store_id]->size(); ++pos)
    {
      id_store_[(*trust_stores_[store_id])[pos].peer_identity] = CalculateID(store_id, pos);
    }
  }

  static time_t GetCurrentTime()
  {
    return std::time(nullptr);
  }

protected:
  mutable bool          dirty_ = false;
  mutable RMutex rmutex_;//{__LINE__, __FILE__};

  mutable TrustStore  good_trust_store_;
  mutable TrustStore  bad_trust_store_;
  mutable TrustStore  buffer_trust_store_;
  mutable IDStore     id_store_;
  
  std::array<TrustStore*, 3> trust_stores_{{
      &buffer_trust_store_, 
      &good_trust_store_, 
      &bad_trust_store_
  }};


  /**
   * Constants
   */
  double const beta             = 100 / 12.;
  double const drift            = 1 / 6.;
  double const eps              = 0.2;
  double const mu_new_peer      = 100.;
  double const sigma_new_peer   = 100./6.;
  double const score_threshold_ = 20.0;
  double const sigma_threshold_ = 13.0;


  size_t const MAX_GOOD_TRUST_STORE_SIZE   = 1000;  // max 65535
  size_t const MAX_BAD_TRUST_STORE_SIZE    = 1000;  // max 65535
  size_t const MAX_BUFFER_TRUST_STORE_SIZE = 1000;  // max 65535
};

}  // namespace p2p
}  // namespace fetch
