#pragma once

namespace fetch
{
namespace consensus
{

class SynergeticScheduler
{
public:
  struct WorkItem 
  {
    int64_t time;
    Work work;
  }

  using ContractAddress = byte_array::ConstByteArray;
  using WorkQueue = std::priority_queue< WorkItem >;
  using DAG = ledger::DAG;
  using AddressSet = std::unordered_set< ContractAddress >;

  SynergeticScheduler(DAG &dag)
  : dag_(dag)
  , miner_(dag)
  {
  }

  void ScheduleWorkValidation(Work const &work)
  {
    work_items_.push_back(work);
  }

  void ClearWorkPool()
  {
    // TODO.
  }

  void CreateContract()
  {
    if(!contract_register.CreateContract("0xf232", source))
    {
      std::cout << "Could not create contract."  << std::endl;
      return;
    }    
  }

  void WorkCycle()  // TODO: Bad name
  {
    if(work_items_.size() == 0)
    {
      // Nothing todo
      return;
    }

    Block block = work_items_.front();
    work_items_.pop_front();

    std::queue< DAGNode > work_to_validate = dag_.GetNodes();
    // TODO, sort firstly with respect to contract name and secondly with respect to quality
    if(work_to_validate.size() == 0)
    {
      // Nothing todo
      return;
    }

    Work work = work_to_validate.front();


    work.contract_address = "0xf232";
    work.miner = "troels";

    if(!miner.DefineProblem(cregister.GetContract(work.contract_address), work))
    {
      std::cout << "Could not define problem!" << std::endl;
      exit(-1);
    }

    // Let's mine
    for(int64_t i = 0; i < 10; ++i) {
      work.nonce = 29188 + i;
      work.score = miner.ExecuteWork(cregister.GetContract(work.contract_address), work);
      work_register_.RegisterWork(work);
    }

    work_register_.ClearWorkPool( cregister.GetContract(work.contract_address) );    
  }
private:
  DAG &dag_;
  fetch::consensus::SynergeticMiner miner_;  
  WorkQueue work_items_;
  WorkItem current_work_;

  SynergeticContractRegister contract_register_;
  fetch::consensus::WorkRegister work_register_;  
};

}
}