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
    WorkAddress address = work.contract_address;

    if(work_pool_.find(address) == work_pool_.end())
    {
      work_pool_[address] = work;
      return;
    }

    auto cur_work = work_pool_[address];
    if(cur_work.score > work.score)
    {
      work_pool_[address] = work;      
    }
  }

  void ClearWorkPool(SynergeticContract contract)
  {
    WorkAddress address = contract->address;

    auto it = work_pool_.find(address);    
    if(it == work_pool_.end())
    {
      // TODO: Nothing to do
      return;
    }

    std::cout << "Invoke clear contract for " << address << std::endl;
    std::cout << "Work score: " << it->second.score << std::endl;
    std::cout << "Nonce: " << it->second() << std::endl;
  }
private:
  std::unordered_map< WorkAddress, Work > work_pool_;
};

}
}