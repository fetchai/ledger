#pragma once

#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/muddle/subscription.hpp"

#include "beacon_round.hpp"
#include "cabinet_member_details.hpp"
#include "entropy.hpp"

namespace fetch {
namespace beacon {

class BeaconSetupService
{
public:
  enum class State
  {
    IDLE = 0,
    BROADCAST_ID,
    WAIT_FOR_IDS,
    CREATE_SHARES,
    SEND_SHARES,
    WAIT_FOR_SHARES,
    GENERATE_KEYS,
    BEACON_READY
    // TODO: Support for complaints and quals
  };

  // Protocol call numbers
  enum
  {
    SUBMIT_SHARE
  };

  using BeaconManager      = dkg::BeaconManager;
  using Identity           = crypto::Identity;
  using SharedBeacon       = std::shared_ptr<BeaconRoundDetails>;
  using CallbackFunction   = std::function<void(SharedBeacon)>;
  using Endpoint           = muddle::MuddleEndpoint;
  using Muddle             = muddle::Muddle;
  using Client             = muddle::rpc::Client;
  using ClientPtr          = std::shared_ptr<Client>;
  using StateMachine       = core::StateMachine<State>;
  using StateMachinePtr    = std::shared_ptr<StateMachine>;
  using ConstByteArray     = byte_array::ConstByteArray;
  using SubscriptionPtr    = muddle::MuddleEndpoint::SubscriptionPtr;
  using Serializer         = serializers::MsgPackSerializer;
  using Address            = byte_array::ConstByteArray;
  using PrivateKey         = BeaconManager::PrivateKey;
  using VerificationVector = BeaconManager::VerificationVector;

  struct ShareSubmission
  {
    Identity           from{};
    PrivateKey         share{};
    VerificationVector verification_vector{};
  };

  struct DeliveryDetails
  {
    bool             was_delivered{false};
    service::Promise response{nullptr};
  };

  BeaconSetupService(Endpoint &endpoint, Identity identity);

  State OnIdle();
  State OnBroadcastID();
  State WaitForIDs();
  State CreateShares();
  State SendShares();
  bool  SubmitShare(Identity from, PrivateKey share, VerificationVector verification_vector);
  State OnWaitForShares();
  State OnGenerateKeys();
  State OnBeaconReady();
  void  QueueSetup(SharedBeacon beacon);

  // TODO: ... steps - rbc? ...
  // TODO: support for complaints

  void SetBeaconReadyCallback(CallbackFunction callback);

  std::weak_ptr<core::Runnable> GetWeakRunnable();

private:
  Identity            identity_;
  Endpoint &          endpoint_;
  SubscriptionPtr     share_subscription_;
  SubscriptionPtr     id_subscription_;
  muddle::rpc::Client rpc_client_;

  std::mutex               mutex_;
  CallbackFunction         callback_function_;
  std::deque<SharedBeacon> beacon_queue_;
  SharedBeacon             beacon_;

  std::shared_ptr<StateMachine>                   state_machine_;
  std::vector<CabinetMemberDetails>               member_details_queue_;
  std::unordered_map<Identity, BeaconManager::Id> member_details_;

  std::unordered_map<Identity, DeliveryDetails> share_delivery_details_;
  std::unordered_map<Identity, ShareSubmission> submitted_shares_;
};

}  // namespace beacon
}  // namespace fetch