#ifndef P2PTRUST_INTERFACE_HPP
#define P2PTRUST_INTERFACE_HPP

#include <iostream>
#include <string>

namespace fetch
{
namespace p2p
{

typedef enum {
  BLOCK = 0,
  TRANSACTION = 1,
  PEER = 2
} P2PTrustFeedbackSubject;

typedef enum {
  LIED = 0,
  BAD_CONNECTION = 1,
  DUPLICATE = 2,
  NEW_INFORMATION = 3
} P2PTrustFeedbackQuality;

template< class PEER_IDENT >
class P2PTrustInterface
{
public:
  P2PTrustInterface(const P2PTrustInterface &rhs)            = delete;
  P2PTrustInterface(P2PTrustInterface &&rhs)                 = delete;
  P2PTrustInterface &operator=(const P2PTrustInterface &rhs) = delete;
  P2PTrustInterface &operator=(P2PTrustInterface &&rhs)      = delete;
  bool operator==(const P2PTrustInterface &rhs) const        = delete;
  bool operator<(const P2PTrustInterface &rhs) const         = delete;

  explicit P2PTrustInterface()
  {
  }

  virtual ~P2PTrustInterface()
  {
  }

  virtual void AddFeedback(const PEER_IDENT &peer_ident,
                           const byte_array::ConstByteArray &object_ident,
                           P2PTrustFeedbackSubject subject,
                           P2PTrustFeedbackQuality quality
                           ) = 0;

  virtual std::vector<PEER_IDENT> GetBestPeers(size_t maximum) = 0;
  virtual size_t                  GetRankOfPeer(const PEER_IDENT &peer_ident) = 0;
  virtual double                  GetTrustRatingOfPeer(const PEER_IDENT &peer_ident) = 0;
  virtual bool                    IsPeerTrusted(const PEER_IDENT &peer_ident) = 0;
};

}
}

#endif //P2PTRUST_INTERFACE_HPP
