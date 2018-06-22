#ifndef SWARM_PEER_LOCATION__
#define SWARM_PEER_LOCATION__

#include <iostream>
#include <string>
#include <list>

namespace fetch
{
namespace swarm
{

class SwarmPeerLocation
{
public:
  static std::list<SwarmPeerLocation> ParsePeerListString(const std::string &s)
  {
    auto results = std::list<SwarmPeerLocation>();
    size_t p = 0;

    while(p<s.length())
      {
        size_t np = s.find(",", p);
        auto subs = s.substr(p, np-p);
        if (subs.length() > 0)
          {
            results.push_back(SwarmPeerLocation(subs));
          }
        if (np == std::string::npos)
          break;
        p = np + 1; // skip the bit we used AND the comma.
      }

    return results;
  }

  SwarmPeerLocation(const std::string &locn)
  {
    locn_ = locn;
  }

  SwarmPeerLocation(const SwarmPeerLocation &rhs)
  {
    locn_ = rhs.locn_;
  }

  SwarmPeerLocation(SwarmPeerLocation &&rhs)
  {
    locn_ = std::move(rhs.locn_);
  }

  SwarmPeerLocation &operator=(const SwarmPeerLocation &rhs)
  {
    locn_ = rhs.locn_;
    return *this;
  }

  SwarmPeerLocation &operator=(SwarmPeerLocation &&rhs)
  {
    locn_ = std::move(rhs.locn_);
    return *this;
  }

  bool operator==(const SwarmPeerLocation &other) const
  {
    return this->locn_ == other.locn_;
  }

  bool operator!=(const SwarmPeerLocation &other) const
  {
    return this->locn_ != other.locn_;
  }

  bool operator<(const SwarmPeerLocation &other) const
  {
    return this->locn_ < other.locn_;
  }

  virtual ~SwarmPeerLocation()
  {
  }

  std::string GetHost() const
  {
    size_t colon_pos = locn_.find(":");
    if (std::string::npos == colon_pos)
      {
        return std::string(locn_);
      }
    return locn_.substr(0, colon_pos);
  }

  uint16_t GetPort() const
  {
    size_t colon_pos = locn_.find(":");
    if (std::string::npos == colon_pos)
      {
        return 9001;
      }
    return uint16_t(std::stoi(locn_.substr(colon_pos+1)));
  }

  const std::string &AsString() const
  {
    return locn_;
  }

private:
  std::string locn_;
};

}
}

#endif //__SWARM_PEER_LOCATION__
