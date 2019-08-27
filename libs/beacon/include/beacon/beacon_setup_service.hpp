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

#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "muddle/subscription.hpp"

#include "beacon/aeon.hpp"
#include "beacon/cabinet_member_details.hpp"
#include "beacon/entropy.hpp"

namespace fetch {
namespace beacon {

class BeaconSetupService
{
public:
  static constexpr char const *LOGGING_NAME = "BeaconSetupService";

  enum class State
  {
    IDLE = 0,
    WAIT_FOR_DIRECT_CONNECTIONS,

    BROADCAST_ID,
    WAIT_FOR_IDS,

    CREATE_SHARES,
    SEND_SHARES,
    WAIT_FOR_SHARES,

    /*
        // TODO(tfr): Instate qual support
        BROADCAST_COMPLAINTS,
        WAIT_FOR_COMPLAINT_ANSWERS,

        BROADCAST_QUAL_SHARES,
        WAIT_FOR_QUAL_COMPLAINTS,
    */

    GENERATE_KEYS,
    BEACON_READY
  };

  using BeaconManager           = dkg::BeaconManager;
  using Identity                = crypto::Identity;
  using SharedAeonExecutionUnit = std::shared_ptr<AeonExecutionUnit>;
  using CallbackFunction        = std::function<void(SharedAeonExecutionUnit)>;
  using Endpoint                = muddle::MuddleEndpoint;
  using Client                  = muddle::rpc::Client;
  using ClientPtr               = std::shared_ptr<Client>;
  using StateMachine            = core::StateMachine<State>;
  using StateMachinePtr         = std::shared_ptr<StateMachine>;
  using ConstByteArray          = byte_array::ConstByteArray;
  using SubscriptionPtr         = muddle::MuddleEndpoint::SubscriptionPtr;
  using Serializer              = serializers::MsgPackSerializer;
  using Address                 = byte_array::ConstByteArray;
  using PrivateKey              = BeaconManager::PrivateKey;
  using VerificationVector      = BeaconManager::VerificationVector;

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

  BeaconSetupService()                           = delete;
  BeaconSetupService(BeaconSetupService const &) = delete;
  BeaconSetupService(BeaconSetupService &&)      = delete;
  BeaconSetupService(Endpoint &endpoint, Identity identity);

  /// State functions
  /// @{
  State OnIdle();
  State OnWaitForDirectConnections();
  State OnBroadcastID();
  State WaitForIDs();
  State CreateShares();
  State SendShares();

  State OnWaitForShares();
  State OnGenerateKeys();
  State OnBeaconReady();
  /// @}

  /// Protocol calls
  /// @{
  bool SubmitShare(Identity from, PrivateKey share, VerificationVector verification_vector);
  /// @}

  /// Setup management
  /// @{
  void QueueSetup(SharedAeonExecutionUnit beacon);
  void SetBeaconReadyCallback(CallbackFunction callback);
  /// @}

  std::weak_ptr<core::Runnable> GetWeakRunnable();

private:
  Identity            identity_;
  Endpoint &          endpoint_;
  SubscriptionPtr     share_subscription_;
  SubscriptionPtr     id_subscription_;
  muddle::rpc::Client rpc_client_;

  std::mutex                          mutex_;
  CallbackFunction                    callback_function_;
  std::deque<SharedAeonExecutionUnit> aeon_exe_queue_;
  SharedAeonExecutionUnit             beacon_;

  std::shared_ptr<StateMachine>                   state_machine_;
  std::vector<CabinetMemberDetails>               member_details_queue_;
  std::unordered_map<Identity, BeaconManager::Id> member_details_;

  std::unordered_map<Identity, DeliveryDetails> share_delivery_details_;
  std::unordered_map<Identity, ShareSubmission> submitted_shares_;
};

}  // namespace beacon
}  // namespace fetch
