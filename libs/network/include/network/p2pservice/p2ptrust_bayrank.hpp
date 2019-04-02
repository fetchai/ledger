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

using reference_players_type = std::array<math::statistics::Gaussian<double>, 4>;
extern const reference_players_type reference_players_;

inline math::statistics::Gaussian<double> const &LookupReferencePlayer(TrustQuality quality)
{
  return reference_players_.at(static_cast<std::size_t>(quality));
}

template <typename IDENTITY>
class P2PTrustBayRank : public P2PTrustInterface<IDENTITY>
{
protected:
  const double threshold_ = 20.0;
  using Gaussian          = math::statistics::Gaussian<double>;
  struct PeerTrustRating
  {
    IDENTITY peer_identity;
    Gaussian g;
    double   score;
    void     update_score()
    {
      score = g.mu() - 3 * g.sigma();
    }
    bool scored = false;
  };
  using TrustStore   = std::vector<PeerTrustRating>;
  using RankingStore = std::unordered_map<IDENTITY, size_t>;
  using Mutex        = mutex::Mutex;
  using PeerTrusts   = typename P2PTrustInterface<IDENTITY>::PeerTrusts;

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

  void AddFeedback(IDENTITY const &peer_ident, ConstByteArray const & /*object_ident*/,
                   TrustSubject subject, TrustQuality quality) override
  {
    FETCH_UNUSED(subject);
    FETCH_LOCK(mutex_);

    auto ranking = ranking_store_.find(peer_ident);

    size_t pos;
    if (ranking == ranking_store_.end())
    {
      PeerTrustRating new_record{peer_ident, Gaussian::ClassicForm(100., 100 / 6.), 0, false};
      pos = trust_store_.size();
      trust_store_.push_back(new_record);
    }
    else
    {
      pos = ranking->second;
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Feedback: ", byte_array::ToBase64(peer_ident),
                    " subj=", ToString(subject), " qual=", ToString(quality));
    if (quality == TrustQuality::NEW_PEER)
    {
      trust_store_[pos].update_score();
      dirty_ = true;
      SortIfNeeded();
      return;  // we're introducing this element, not rating it.
    }

    Gaussian const &reference_player = LookupReferencePlayer(quality);
    bool honest = quality == TrustQuality::NEW_INFORMATION || quality == TrustQuality::DUPLICATE;
    trust_store_[pos].scored = true;
    updateGaussian(honest, trust_store_[pos].g, reference_player, 100 / 12., 1 / 6., 0.2);
    trust_store_[pos].update_score();

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
    if (maximum_count > trust_store_.size())
    {
      return GetBestPeers(maximum_count);
    }

    IdentitySet result;
    result.reserve(maximum_count);

    size_t                                max_trial = maximum_count * 1000;
    std::random_device                    rd;
    std::mt19937                          g(rd());
    std::uniform_int_distribution<size_t> distribution(0, trust_store_.size() - 1);

    {
      FETCH_LOCK(mutex_);

      for (std::size_t i = 0, pos = 0, inserted_element_counter = 0; i < max_trial; ++i)
      {
        pos = distribution(g);
        if (trust_store_[pos].score < minimum_trust)
        {
          continue;
        }

        result.insert(trust_store_[pos].peer_identity);
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
    result.reserve(maximum);

    {
      FETCH_LOCK(mutex_);

      for (std::size_t pos = 0, end = std::min(maximum, trust_store_.size()); pos < end; ++pos)
      {
        if (trust_store_[pos].score < threshold_)
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

  PeerTrusts GetPeersAndTrusts() const override
  {
    FETCH_LOCK(mutex_);
    PeerTrusts trust_list;

    for (std::size_t pos = 0, end = trust_store_.size(); pos < end; ++pos)
    {
      PeerTrust pt;
      pt.address        = trust_store_[pos].peer_identity;
      pt.name           = std::string(byte_array::ToBase64(pt.address));
      pt.trust          = trust_store_[pos].score;
      pt.has_transacted = trust_store_[pos].scored;
      trust_list.push_back(pt);
    }

    return trust_list;
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
        ranking = trust_store_[ranking_it->second].score;
      }
    }

    return ranking;
  }

  bool IsPeerTrusted(IDENTITY const &peer_ident) const override
  {
    return GetTrustRatingOfPeer(peer_ident) > threshold_;
  }

  virtual void Debug() const override
  {
    FETCH_LOCK(mutex_);
    for (std::size_t pos = 0; pos < trust_store_.size(); ++pos)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "trust_store_ ",
                     byte_array::ToBase64(trust_store_[pos].peer_identity), " => ",
                     trust_store_[pos].score);
    }
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

  void updateGaussian(bool honest, Gaussian &s, Gaussian const &ref, double beta, double drift,
                      double eps)
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

  void SortIfNeeded()
  {
    if (!dirty_)
    {
      return;
    }
    dirty_ = false;

    std::sort(trust_store_.begin(), trust_store_.end(),
              [](const PeerTrustRating &a, const PeerTrustRating &b) {
                if (a.score < b.score)
                {
                  return true;
                }
                if (a.score > b.score)
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

protected:
  bool          dirty_ = false;
  mutable Mutex mutex_{__LINE__, __FILE__};
  TrustStore    trust_store_;
  RankingStore  ranking_store_;
};

}  // namespace p2p
}  // namespace fetch
