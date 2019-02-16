#pragma once

#include "work.hpp"

#include <vector>
#include <unordered_map>

namespace fetch
{
namespace consensus
{

class WorkRegister 
{
public:
  using WorkAddress = byte_array::ConstByteArray;

  void RegisterWork(Work const &work)
  {
    // TODO: Extend with work id.
    WorkAddress address = work.contract_address;

  }

  void ClearWorkPool(SynergeticContract contract)
  {
    WorkAddress address = contract.contract_address;
  }
private:
  std::unordered_map< WorkAddress, Work > work_pool_;
}
}
}