#include "crypto/bls_base.hpp"
#include "crypto/bls_dkg.hpp"
#include "dkg/beacon_manager.hpp"

#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>

struct BeaconRoundDetails
{
  BeaconManager manager{};
  std::unordered_set< Identity > members{};
  uint32_t round{0};
  std::atomic< bool > ready{false};
};



class BeaconSetupService
{
public:
  using SharedBeacon     = std::shared_ptr<BeaconRoundDetails>;  
  using CallbackFunction = std::function< void(SharedBeacon) >;

  enum class State
  {
    IDLE              = 0
    BROADCAST_ID      ,
    WAIT_FOR_IDS,
    CREATE_AND_SEND_SHARES,
    WAIT_FOR_SHARES,
    GENERATE_KEYS,
    BEACON_READY
    // TODO: Support for complaints and valid nodes
  };

  BeaconSetupService() 
  {
    state_machine_->RegisterHandler(State::IDLE, this, &BeaconSetupService::OnIdle);
    state_machine_->RegisterHandler(State::BROADCAST_ID, this, &BeaconSetupService::OnBroadcastID);
    state_machine_->RegisterHandler(State::WAIT_FOR_IDS, this, &BeaconSetupService::WaitForIDs);
    state_machine_->RegisterHandler(State::CREATE_AND_SEND_SHARES, this, &BeaconSetupService::CreateAndSendShares);
    state_machine_->RegisterHandler(State::WAIT_FOR_SHARES, this, &BeaconSetupService::OnWaitForShares);
    state_machine_->RegisterHandler(State::GENERATE_KEYS, this, &BeaconSetupService::OnGenerateKeys);    
    state_machine_->RegisterHandler(State::BEACON_READY, this, &BeaconSetupService::OnBeaconReady);    
  }

  State OnIdle()
  {

  }

  State OnBroadcastID()
  {

  }

  State WaitForIDs()
  {

  }

  State CreateAndSendShares()
  {

  }

  State OnWaitForShares()
  {

  }

  State OnGenerateKeys()
  {

  }

  State OnBeaconReady()
  {

  }



  void QueueSetup(SharedBeacon beacon)
  {
    std::lock_guard< std::mutex > lock(mutex_);
    beacon_queue_.push_back(beacon);
  }

  // TODO: ... steps - rbc? ...
  // TODO: support for complaints

  void Final()
  {
    std::lock_guard< std::mutex > lock(mutex_);
    if(callback_function_)
    {
      callback_function_(next_beacon_)
    }

  }

  void SetBeaconReadyCallback(CallbackFunction callback)
  {
    std::lock_guard< std::mutex > lock(mutex_);
    callback_function_ = callback;
  }
private:
  std::mutex mutex_;
  CallbackFunction callback_function_;
  std::deque< SharedBeacon > beacon_queue_;
  SharedBeacon next_beacon_;

  MuddleEndpoint &                      muddle_endpoint_;
  ClientPtr                             client_;
  std::shared_ptr<StateMachine>         state_machine_;
};

class BeaconService
{
public:
  using Prover         = fetch::crypto::Prover;
  using ProverPtr      = std::shared_ptr<Prover>;
  using Certificate    = fetch::crypto::Prover;
  using CertificatePtr = std::shared_ptr<Certificate>;
  using Address        = fetch::muddle::Packet::Address;
  using BeaconManager  = fetch::crypto::BeaconManager;
  using SharedBeacon   = std::shared_ptr<BeaconRoundDetails>;

  BeaconService() 
  {
    cabinet_creator_,OnBeaconReady([this](SharedBeacon beacon)
    {
      std::lock_guard< std::mutex > lock(mutex_);
      beacon_queue_.push_back(beacon);
    })
  }

  /// Maintainance logic
  /// @{
  /* @brief this function is called when the node is in the cabinet
   *
   */
  void StartNewCabinet(std::unordered_set< Identity > members, uint32_t threshold)
  {
    std::lock_guard< std::mutex > lock(mutex_);

    SharedBeacon beacon = std::make_shared< BeaconRoundDetails >();

    beacon->manager.Reset(members.size(), threshold);
    beacon->round   = next_cabinet_generation_number_;
    beacon->members = std::move(members);

    cabinet_creator_.QueueSetup(beacon);
    ++next_cabinet_generation_number_;
  }

  /* @brief this function is called when the node is not.
  */
  void SkipRound()
  {
    std::lock_guard< std::mutex > lock(mutex_);
    ++next_cabinet_generation_number_;
  }

  bool SwitchCabinet()
  {
    std::lock_guard< std::mutex > lock(mutex_);
    if(beacon_queue_.size() == 0)
    {
      return false;
    }

    auto f = beacon_queue_.front();
    active_beacon_.reset();

    if(f->round == next_cabinet_number_)
    {
      active_beacon_ = f;
    }

    ++next_cabinet_number_;
  }

  /// @}


private:
  std::mutex mutex_;  
  std::shared_ptr<BeaconRoundDetails> active_beacon_;

  uint64_t next_cabinet_generation_number_{0};
  uint64_t next_cabinet_number_{0};

  std::deque< SharedBeacon > beacon_queue_;

  BeaconSetupService cabinet_creator_;
};

int main()
{

  return 0;
}