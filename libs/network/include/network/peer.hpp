#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
  Peer(std::string address, uint16_t port) : address_{std::move(address)}, port_{port} {}
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
