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

#include "fetch_asio.hpp"

#include <iostream>
#include <utility>

namespace fetch {
namespace network {

/**
 * Representation of an network adapter. I.e. an IP address and network mask of
 * one of the system network adapter cards.
 *
 * This is to be used to select which interfaces a service should be bound to.
 */
class Adapter
{
public:
  using address_type      = asio::ip::address_v4;
  using adapter_list_type = std::vector<Adapter>;

  static adapter_list_type GetAdapters();

  Adapter(address_type address, address_type network_mask)
    : address_{std::move(address)}
    , network_mask_{std::move(network_mask)}
  {}
  Adapter(Adapter const &) = default;
  ~Adapter()               = default;

  address_type const &address() const
  {
    return address_;
  }

  address_type const &network_mask() const
  {
    return network_mask_;
  }

private:
  address_type address_;
  address_type network_mask_;
};

}  // namespace network
}  // namespace fetch
