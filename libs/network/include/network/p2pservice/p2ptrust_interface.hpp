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
#include "network/muddle/muddle.hpp"
#include "variant/variant.hpp"

#include <list>
#include <string>
#include <unordered_set>

namespace fetch {
namespace p2p {

enum class TrustSubject
{
  BLOCK       = 0,
  TRANSACTION = 1,
  PEER        = 2
};

enum class TrustQuality
{
  LIED            = 0,
  BAD_CONNECTION  = 1,
  DUPLICATE       = 2,
  NEW_INFORMATION = 3,
  NEW_PEER        = 4,
};

template <typename IDENTITY>
class P2PTrustInterface
{
public:
  struct PeerTrust
  {
    IDENTITY    address;
    std::string name;
    double      trust;
    bool        has_transacted;
    bool        active;
  };
  using PeerTrusts = std::vector<PeerTrust>;

  using IdentitySet    = typename std::unordered_set<IDENTITY>;
  using ConstByteArray = byte_array::ConstByteArray;

  P2PTrustInterface(const P2PTrustInterface &rhs) = delete;
  P2PTrustInterface(P2PTrustInterface &&rhs)      = delete;
  P2PTrustInterface &operator=(const P2PTrustInterface &rhs) = delete;
  P2PTrustInterface &operator=(P2PTrustInterface &&rhs)             = delete;
  bool               operator==(const P2PTrustInterface &rhs) const = delete;
  bool               operator<(const P2PTrustInterface &rhs) const  = delete;

  P2PTrustInterface()          = default;
  virtual ~P2PTrustInterface() = default;

  virtual void AddFeedback(IDENTITY const &peer_ident, TrustSubject subject,
                           TrustQuality quality) = 0;

  virtual void AddFeedback(IDENTITY const &peer_ident, ConstByteArray const &object_ident,
                           TrustSubject subject, TrustQuality quality) = 0;

  virtual IdentitySet GetBestPeers(size_t maximum) const = 0;
  virtual PeerTrusts  GetPeersAndTrusts() const          = 0;

  virtual IdentitySet GetRandomPeers(size_t maximum_count, double minimum_trust) const = 0;

  virtual std::size_t GetRankOfPeer(IDENTITY const &peer_ident) const        = 0;
  virtual double      GetTrustRatingOfPeer(IDENTITY const &peer_ident) const = 0;
  virtual bool        IsPeerTrusted(IDENTITY const &peer_ident) const        = 0;
  virtual bool        IsPeerKnown(IDENTITY const &peer_ident) const          = 0;

  virtual void Debug() const = 0;
};

inline char const *ToString(TrustSubject subject)
{
  switch (subject)
  {
  case TrustSubject::BLOCK:
    return "Block";
  case TrustSubject::TRANSACTION:
    return "Transaction";
  case TrustSubject::PEER:
    return "Peer";
  default:
    return "Unknown";
  }
}

inline char const *ToString(TrustQuality quality)
{
  switch (quality)
  {
  case TrustQuality::LIED:
    return "Lied";
  case TrustQuality::BAD_CONNECTION:
    return "Bad Connection";
  case TrustQuality::DUPLICATE:
    return "Duplicate";
  case TrustQuality::NEW_INFORMATION:
    return "New Information";
  case TrustQuality::NEW_PEER:
    return "New Peer";
  default:
    return "Unknown";
  }
}

}  // namespace p2p
}  // namespace fetch
