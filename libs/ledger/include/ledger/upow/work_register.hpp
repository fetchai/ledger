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
  using ContractName = byte_array::ConstByteArray;

  void RegisterWork(Work const &work)
  {
    ContractName name = work.contract_name;
    if(work_pool_.find(name) == work_pool_.end())
    {
      work_pool_[name] = work;
      return;
    }

    auto cur_work = work_pool_[name];
    if(cur_work.score > work.score)
    {
      work_pool_[name] = work;      
    }
  }

  Work ClearWorkPool(SynergeticContract contract)
  {
    ContractName name = contract->name;

    auto it = work_pool_.find(name);    
    if(it == work_pool_.end())
    {
      return {};
    }

    Work ret = it->second;
    work_pool_.erase(it);
    return ret;
  }
private:
  std::unordered_map< ContractName, Work > work_pool_;
};

}
}