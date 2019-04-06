#pragma once
#include "ledger/upow/work.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/identifier.hpp"

#include <algorithm>
#include <queue>
#include <memory>
namespace fetch
{
namespace consensus
{

struct WorkPackage
{
  using ContractName         = byte_array::ConstByteArray;
  using ContractAddress      = storage::ResourceAddress;  
  using SharedWorkPackage    = std::shared_ptr< WorkPackage >;  
  using WorkQueue            = std::priority_queue< Work >;
  
  static SharedWorkPackage New(ContractName name, ContractAddress contract_address) {
    return SharedWorkPackage(new WorkPackage(std::move(name), std::move(contract_address)));
  }

  /// @name defining work and candidate solutions
  /// @{
  ContractName    contract_name{};    //< Contract name which is extracted from the DAG
  ContractAddress contract_address{}; //< The corresponding address of the contract
  WorkQueue       solutions_queue{};  //< A prioritised queue with work items
  /// }

  // Work packages needs to be sortable to ensure consistent execution
  // order accross different platforms.
  bool operator<(WorkPackage const &other) const 
  {
    return contract_address < other.contract_address;
  }
private:
  WorkPackage(ContractName name, ContractAddress address) 
  : contract_name{std::move(name)}
  , contract_address{std::move(address)} { }
};

}
}