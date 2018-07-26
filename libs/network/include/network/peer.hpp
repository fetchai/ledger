#pragma once

#include <cstdint>
#include <iostream>
#include <string>

namespace fetch {
namespace network {

class Peer
{
public:
  // Construction / Destruction
  Peer() = default;
  Peer(std::string const &address);
  Peer(std::string address, uint16_t port)
      : address_{std::move(address)}, port_{port}
  {}
  Peer(Peer const &) = default;
  Peer(Peer &&)      = default;
  ~Peer()            = default;

  bool Parse(std::string const &address);
  void Update(std::string address, uint16_t port)
  {
    address_ = std::move(address);
    port_    = port;
  }

  std::string const &address() const { return address_; }

  uint16_t port() const { return port_; }

  Peer &operator=(Peer const &) = default;
  Peer &operator=(Peer &&) = default;

  friend std::ostream &operator<<(std::ostream &s, Peer const &peer)
  {
    s << peer.address_ << ':' << peer.port_;
    return s;
  }

private:
  std::string address_{"localhost"};
  uint16_t    port_{0};
};

}  // namespace network
}  // namespace fetch

