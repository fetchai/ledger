#ifndef P2PTRUST_HPP
#define P2PTRUST_HPP

#include "core/byte_array/const_byte_array.hpp"
#include "core/mutex.hpp"
#include "network/p2pservice/p2ptrust_interface.hpp"

#include <array>
#include <cmath>
#include <ctime>
#include <iostream>
#include <map>
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
  TrustModifier2() { this->delta = 19.73; }
  TrustModifier2(double delta, double min, double max)
  {
    this->delta = delta;
    this->min   = min;
    this->max   = max;
  }
  double delta, min, max;

  ~TrustModifier2() {}
};

using trust_modifiers_type = std::array<std::array<TrustModifier, 4>, 3>;
extern const trust_modifiers_type trust_modifiers_;

template <class PEER_IDENT>
class P2PTrust : public P2PTrustInterface<PEER_IDENT>
{
protected:
  struct PeerTrustRating
  {
    PEER_IDENT peer_ident;
    double     trust;
    time_t     lastmodified;

    double computeCurrentTrust(time_t currenttime)
    {
      auto time_delta = double(std::max(0L, lastmodified + 100 - currenttime)) / 100.0;
      return trust * time_delta;
    }

    void SetCurrentTrust(time_t currenttime)
    {
      trust        = computeCurrentTrust(currenttime);
      lastmodified = currenttime;
    }
  };

  using trust_store_type   = std::vector<PeerTrustRating>;
  using ranking_store_type = std::map<PEER_IDENT, size_t>;
  using mutex_type         = mutex::Mutex;
  using lock_type          = std::lock_guard<mutex_type>;

public:
  P2PTrust(const P2PTrust &rhs) = delete;
  P2PTrust(P2PTrust &&rhs)      = delete;
  P2PTrust operator=(const P2PTrust &rhs) = delete;
  P2PTrust operator=(P2PTrust &&rhs)             = delete;
  bool     operator==(const P2PTrust &rhs) const = delete;
  bool     operator<(const P2PTrust &rhs) const  = delete;

  explicit P2PTrust() : P2PTrustInterface<PEER_IDENT>() {}

  virtual ~P2PTrust() {}

  void sortWillBeNeeded() { dirty_ = true; }

  virtual void AddFeedback(const PEER_IDENT &                peer_ident,
                           const byte_array::ConstByteArray &object_ident,
                           P2PTrustFeedbackSubject           subject,
                           P2PTrustFeedbackQuality           quality) override
  {
    lock_type lock(mutex_);
    sortWillBeNeeded();

    auto ranking     = ranking_store_.find(peer_ident);
    auto currenttime = getCurrentTime();

    size_t pos;
    if (ranking == ranking_store_.end())
    {
      PeerTrustRating new_record{peer_ident, 0.0, currenttime};
      pos = trust_store_.size();
      trust_store_.push_back(new_record);
    }
    else
    {
      pos = ranking->second;
    }

    auto update = fetch::p2p::trust_modifiers_[subject][quality];
    auto trust  = trust_store_[pos].computeCurrentTrust(currenttime);

    if ((std::isnan(update.max) || (trust < update.max)) &&
        (std::isnan(update.min) || (trust > update.min)))
    {
      trust += update.delta;
    }

    trust_store_[pos].trust        = trust;
    trust_store_[pos].lastmodified = currenttime;
  }

  virtual std::vector<PEER_IDENT> GetBestPeers(size_t maximum) override
  {
    SortIfNeeded();

    std::vector<PEER_IDENT> result;
    result.reserve(maximum);

    lock_type lock(mutex_);
    for (size_t pos = 0; pos < std::min(maximum, trust_store_.size()); pos++)
    {
      if (trust_store_[pos].trust < 0.0)
      {
        break;
      }
      result.push_back(trust_store_[pos].peer_ident);
    }
    return result;
  }

  virtual size_t GetRankOfPeer(const PEER_IDENT &peer_ident) override
  {
    SortIfNeeded();

    lock_type lock(mutex_);

    if (ranking_store_.find(peer_ident) == ranking_store_.end())
    {
      return trust_store_.size() + 1;
    }
    return ranking_store_[peer_ident];
  }

  virtual double GetTrustRatingOfPeer(const PEER_IDENT &peer_ident) override
  {
    auto      currenttime = getCurrentTime();
    lock_type lock(mutex_);
    return trust_store_[ranking_store_[peer_ident]].computeCurrentTrust(currenttime);
  }

  bool IsPeerTrusted(const PEER_IDENT &peer_ident) override
  {
    return GetTrustRatingOfPeer(peer_ident) > 0.0;
  }

protected:
  void SortIfNeeded()
  {
    auto currenttime = getCurrentTime();

    lock_type lock(mutex_);

    if (!dirty_)
    {
      return;
    }
    dirty_ = false;

    for (size_t pos = 0; pos < trust_store_.size(); pos++)
    {
      trust_store_[pos].SetCurrentTrust(currenttime);
    }

    std::sort(trust_store_.begin(), trust_store_.end(), [](const PeerTrustRating &a, const PeerTrustRating &b) {
      if (a.trust < b.trust) return true;
      if (a.trust > b.trust) return false;
      return a.peer_ident < b.peer_ident;
    });

    ranking_store_.clear();

    for (size_t pos = 0; pos < trust_store_.size(); pos++)
    {
      ranking_store_[trust_store_[pos].peer_ident] = pos;
    }
  }

  static time_t getCurrentTime() { return std::time(nullptr); }

private:
  bool       dirty_ = false;
  mutex_type mutex_{__LINE__, __FILE__};

  trust_store_type   trust_store_;
  ranking_store_type ranking_store_;
};

}  // namespace p2p
}  // namespace fetch

#endif  // P2PTRUST_HPP
