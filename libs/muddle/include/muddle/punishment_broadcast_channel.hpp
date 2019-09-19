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

#include "core/byte_array/const_byte_array.hpp"
#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/sha256.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
//#include "muddle/subscription.hpp"
#include "network/service/protocol.hpp"

#include <map>
#include <set>

namespace fetch {
namespace muddle {

/**
 */
class PunishmentBroadcastChannel : public service::Protocol
{
public:
  using CertificatePtr = std::shared_ptr<fetch::crypto::Prover>;
  using Endpoint        = muddle::MuddleEndpoint;
  using ConstByteArray  = byte_array::ConstByteArray;
  using MuddleAddress   = ConstByteArray;
  using CabinetMembers  = std::set<MuddleAddress>;
  using Subscription    = muddle::Subscription;
  using SubscriptionPtr = std::shared_ptr<muddle::Subscription>;
  using HashFunction    = crypto::SHA256;
  using HashDigest      = byte_array::ByteArray;
  using CallbackFunction =
      std::function<void(MuddleAddress const &, byte_array::ConstByteArray const &)>;
  using Server                  = fetch::muddle::rpc::Server;
  using ServerPtr               = std::shared_ptr<Server>;

  // The only RPC function exposed
  enum
  {
    PULL_INFO_FROM_PEER = 1
  };

  uint64_t AllowPeerPull()
  {
    static uint64_t index = 0;
    return index++;
  }

  PunishmentBroadcastChannel(Endpoint &endpoint, MuddleAddress address, CallbackFunction call_back,
      CertificatePtr certificate, uint16_t channel = CHANNEL_RBC_BROADCAST)
    : endpoint_{endpoint}
    , address_{address}
    , deliver_msg_callback_{call_back}
    , channel_{channel}
    , certificate_{certificate}
    , rpc_client_{"PunishmentBC", endpoint_, SERVICE_PBC, channel_}
  {
    this->Expose(PULL_INFO_FROM_PEER, this, &PunishmentBroadcastChannel::AllowPeerPull);

    // TODO(HUT): rpc beacon rename.
    // Attaching the protocol
    rpc_server_ = std::make_shared<Server>(endpoint_, SERVICE_PBC, channel_);
    rpc_server_->Add(RPC_BEACON, this);
  }

  /// RBC Operation
  /// @{
  bool ResetCabinet(CabinetMembers const &/*cabinet*/)
  {
    return true;
  }

  //void Broadcast(SerialisedMessage const &msg)
  //{
  //}

  void Enable(bool /*enable*/)
  {
  }
  /// @}

  Endpoint &          endpoint_;              ///< The muddle endpoint to communicate on
  MuddleAddress const address_;               ///< Our muddle address
  CallbackFunction    deliver_msg_callback_;  ///< Callback for messages which have succeeded
  uint16_t channel_;
  CertificatePtr certificate_;

  ServerPtr           rpc_server_{nullptr};
  muddle::rpc::Client rpc_client_;

  bool enabled_ = true;

  // For broadcast
  CabinetMembers      current_cabinet_;    ///< The set of muddle addresses of the
                                           ///< cabinet (including our own)
  uint32_t threshold_;                     ///< Number of byzantine nodes (this is assumed
                                           ///< to take the maximum allowed value satisying
                                           ///< threshold_ < current_cabinet_.size()
  //SubscriptionPtr rbc_subscription_;       ///< For receiving messages in the rbc channel
  /// @}
};

}  // namespace muddle
}  // namespace fetch

