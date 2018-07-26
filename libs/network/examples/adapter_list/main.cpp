#include "network/adapters.hpp"

#include <iostream>

int main()
{
  std::cout << "System Adapters" << std::endl;

  auto const adapters = fetch::network::Adapter::GetAdapters();

  for (auto const &adapter : adapters)
  {
    std::cout << " - " << adapter.address().to_string()
              << " (mask: " << adapter.network_mask().to_string() << ')'
              << std::endl;
  }

  return 0;
}