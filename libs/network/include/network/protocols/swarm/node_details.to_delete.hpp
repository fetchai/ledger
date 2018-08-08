#pragma once
#include "byte_array/referenced_byte_array.hpp"
#include "chain/transaction.hpp"
#include "mutex.hpp"
#include "protocols/swarm/entry_point.hpp"

namespace fetch {

namespace protocols {

struct NodeDetails
{
  bool operator==(NodeDetails const &other)
  {
    if (public_key != other.public_key)
    {
      return false;
    }

    if (default_port != other.default_port)
    {
      return false;
    }

    if (default_http_port != other.default_http_port)
    {
      return false;
    }

    if (entry_points.size() != other.entry_points.size())
    {
      return false;
    }

    for (std::size_t i = 0; i < entry_points.size(); ++i)
    {
      if (entry_points[i] != entry_points[i])
      {
        return false;
      }
    }

    return true;
  }

  bool operator!=(NodeDetails const &other) { return !(this->operator==(other)); }

  byte_array::ByteArray   public_key;
  std::vector<EntryPoint> entry_points;

  uint16_t default_port;
  uint16_t default_http_port;
};

class SharedNodeDetails
{
public:
  SharedNodeDetails() {}
  SharedNodeDetails(SharedNodeDetails const &) = delete;

  bool operator==(SharedNodeDetails const &other)
  {
    return details_.public_key == other.details_.public_key;
  }

  void AddEntryPoint(EntryPoint const &ep)
  {
    mutex_.lock();
    bool found_ep = false;
    for (auto &e : details_.entry_points)
    {
      if ((e.host == ep.host) && (e.port == ep.port))
      {
        found_ep = true;
        break;
      }
    }
    if (!found_ep) details_.entry_points.push_back(ep);
    mutex_.unlock();
  }

  uint16_t default_port()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    return details_.default_port;
  }

  uint16_t default_http_port()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
    return details_.default_http_port;
  }

  void with_details(std::function<void(NodeDetails &)> fnc)
  {
    mutex_.lock();
    fnc(details_);
    mutex_.unlock();
  }

  NodeDetails details()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(mutex_);

    return details_;
  }

private:
  NodeDetails         details_;
  fetch::mutex::Mutex mutex_;
};
}  // namespace protocols
}  // namespace fetch
