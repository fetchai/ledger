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

#include "network/p2pservice/bayrank/bad_place.hpp"
#include "network/p2pservice/bayrank/buffer.hpp"
#include "network/p2pservice/bayrank/good_place.hpp"

#include <unordered_map>
#include <vector>

namespace fetch {
namespace p2p {
namespace bayrank {

template <typename IDENTITY>
class TheTrain
{
public:
  enum PLACE
  {
    BUFFER = 0,
    GOOD,
    BAD,
    UNKNOWN,
  };

  static constexpr double SCORE_THRESHOLD = 20.;
  static constexpr double SIGMA_THRESHOLD = 13.;

  static constexpr char const *LOGGING_NAME = "TheTrain";

  TheTrain(TrustBuffer<IDENTITY> &buffer, GoodPlace<IDENTITY> &good_place,
           BadPlace<IDENTITY> &bad_place)
    : buffer_(buffer)
    , good_place_(good_place)
    , bad_place_(bad_place)
  {}

  PLACE MoveIfPossible(PLACE place, IDENTITY const &peer_ident)
  {
    switch (place)
    {
    case PLACE::BUFFER:
    {
      return MoveFromBuffer(peer_ident);
    }
    case PLACE::GOOD:
    {
      return MoveFromGoodPlace(peer_ident);
    }
    case PLACE::BAD:
    {
      return MoveFromBadPlace(peer_ident);
    }
    case PLACE::UNKNOWN:
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Got unknown PLACE!");
      return PLACE::UNKNOWN;
    }
    }
  }

private:
  PLACE MoveFromBuffer(IDENTITY const &peer_ident)
  {
    PLACE place   = PLACE::UNKNOWN;
    auto  peer_it = buffer_.GetPeer(peer_ident);
    if (peer_it != buffer_.end())
    {
      place     = PLACE::BUFFER;
      auto peer = *peer_it;
      if (peer.g.sigma() <= SIGMA_THRESHOLD)
      {
        if (peer.score >= SCORE_THRESHOLD)
        {
          good_place_.AddPeer(std::move(peer));
          buffer_.Remove(peer_ident);
          place = PLACE::GOOD;
        }
        else
        {
          bad_place_.AddPeer(std::move(peer));
          buffer_.Remove(peer_ident);
          place = PLACE::BAD;
        }
      }
    }
    return place;
  }

  PLACE MoveFromGoodPlace(IDENTITY const &peer_ident)
  {
    PLACE place   = PLACE::UNKNOWN;
    auto  peer_it = good_place_.GetPeer(peer_ident);
    if (peer_it != good_place_.end())
    {
      place     = PLACE::GOOD;
      auto peer = *peer_it;
      if (peer.g.sigma() >= SIGMA_THRESHOLD || peer.score < SCORE_THRESHOLD)
      {
        buffer_.AddPeer(std::move(peer));
        good_place_.Remove(peer_ident);
        place = PLACE::BUFFER;
      }
    }
    return place;
  }

  PLACE MoveFromBadPlace(IDENTITY const &peer_ident)
  {
    PLACE place   = PLACE::UNKNOWN;
    auto  peer_it = bad_place_.GetPeer(peer_ident);
    if (peer_it != bad_place_.end())
    {
      place     = PLACE::BAD;
      auto peer = *peer_it;
      if (peer.g.sigma() >= SIGMA_THRESHOLD || peer.score > SCORE_THRESHOLD)
      {
        buffer_.AddPeer(std::move(peer));
        bad_place_.Remove(peer_ident);
        place = PLACE::BUFFER;
      }
    }
    return place;
  }

private:
  TrustBuffer<IDENTITY> &buffer_;
  GoodPlace<IDENTITY> &  good_place_;
  BadPlace<IDENTITY> &   bad_place_;
};
}  // namespace bayrank
}  // namespace p2p
}  // namespace fetch