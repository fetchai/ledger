#ifndef P2PTRUST_INTERFACE_HPP
#define P2PTRUST_INTERFACE_HPP

#include "core/byte_array/const_byte_array.hpp"

#include <iostream>
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
  NEW_INFORMATION = 3
};

template <typename IDENTITY>
class P2PTrustInterface
{
public:
  using IdentitySet = typename std::unordered_set<IDENTITY>;
  using ConstByteArray = byte_array::ConstByteArray;

  P2PTrustInterface(const P2PTrustInterface &rhs) = delete;
  P2PTrustInterface(P2PTrustInterface &&rhs)      = delete;
  P2PTrustInterface &operator=(const P2PTrustInterface &rhs) = delete;
  P2PTrustInterface &operator=(P2PTrustInterface &&rhs)             = delete;
  bool               operator==(const P2PTrustInterface &rhs) const = delete;
  bool               operator<(const P2PTrustInterface &rhs) const  = delete;

  P2PTrustInterface() = default;
  virtual ~P2PTrustInterface() = default;

  virtual void AddFeedback(IDENTITY const &peer_ident,
                           TrustSubject subject, TrustQuality quality) = 0;

  virtual void AddFeedback(IDENTITY const &peer_ident,
                           ConstByteArray const &object_ident,
                           TrustSubject subject, TrustQuality quality) = 0;

  virtual IdentitySet GetBestPeers(size_t maximum) const= 0;

  virtual IdentitySet GetRandomPeers(size_t maximum_count, double minimum_trust) const= 0;

  virtual std::size_t GetRankOfPeer(IDENTITY const &peer_ident) const        = 0;
  virtual double      GetTrustRatingOfPeer(IDENTITY const &peer_ident) const = 0;
  virtual bool   IsPeerTrusted(IDENTITY const &peer_ident) const       = 0;
  virtual bool   IsPeerKnown(IDENTITY const &peer_ident) const    = 0;
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
    default:
      return "Unknown";
  }
}

}  // namespace p2p
}  // namespace fetch

#endif  // P2PTRUST_INTERFACE_HPP
