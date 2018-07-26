#pragma once

#include "fetch_asio.hpp"

#include <iostream>

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

  Adapter(address_type const &address, address_type const &network_mask)
      : address_{address}, network_mask_{network_mask}
  {}
  Adapter(Adapter const &) = default;
  ~Adapter()               = default;

  address_type const &address() const { return address_; }

  address_type const &network_mask() const { return network_mask_; }

private:
  address_type address_;
  address_type network_mask_;
};

}  // namespace network
}  // namespace fetch

