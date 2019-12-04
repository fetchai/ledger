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
#include "crypto/ecdsa.hpp"
#include "dmlf/colearn/abstract_message_controller.hpp"
#include "dmlf/colearn/colearn_protocol.hpp"
#include "dmlf/colearn/random_double.hpp"
#include "dmlf/colearn/update_store.hpp"
#include "dmlf/deprecated/abstract_learner_networker.hpp"
#include "dmlf/deprecated/update_interface.hpp"
#include "json/document.hpp"
#include "logging/logging.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "network/service/call_context.hpp"
#include "oef-base/threading/Taskpool.hpp"
#include "oef-base/threading/Threadpool.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class MuddleLearnerNetworkerImpl : public AbstractMessageController
{
public:
  using Taskpool = oef::base::Taskpool;

  using Address                       = fetch::muddle::Address;
  using AlgorithmClass                = AbstractMessageController::AlgorithmClass;
  using Bytes                         = ColearnUpdate::Data;
  using ConstUpdatePtr                = AbstractMessageController::ConstUpdatePtr;
  using Criteria                      = UpdateStoreInterface::Criteria;
  using Lock                          = std::unique_lock<Mutex>;
  using MuddlePtr                     = muddle::MuddlePtr;
  using NetMan                        = fetch::network::NetworkManager;
  using NetManPtr                     = std::shared_ptr<NetMan>;
  using Payload                       = fetch::muddle::Packet::Payload;
  using Peers                         = std::unordered_set<Address>;
  using Proto                         = ColearnProtocol;
  using ProtoPtr                      = std::shared_ptr<ColearnProtocol>;
  using Randomiser                    = RandomDouble;
  using RpcClient                     = fetch::muddle::rpc::Client;
  using RpcClientPtr                  = std::shared_ptr<RpcClient>;
  using RpcServer                     = fetch::muddle::rpc::Server;
  using RpcServerPtr                  = std::shared_ptr<RpcServer>;
  using Signer                        = fetch::crypto::ECDSASigner;
  using SignerPtr                     = std::shared_ptr<Signer>;
  using Sources                       = std::set<std::string>;
  using SourcesList                   = std::vector<std::string>;
  using Store                         = UpdateStore;
  using StorePtr                      = std::shared_ptr<Store>;
  using SubscriptionPtr               = fetch::muddle::MuddleEndpoint::SubscriptionPtr;
  using TaskPtr                       = Taskpool::TaskP;
  using Threadpool                    = oef::base::Threadpool;
  using UpdateClass                   = AbstractMessageController::UpdateClass;
  using UpdatePtr                     = AbstractMessageController::UpdatePtr;
  using UpdateType                    = UpdateStore::UpdateType;
  using Uri                           = fetch::network::Uri;
  using deprecated_UpdateInterfacePtr = dmlf::deprecated_UpdateInterfacePtr;

  static constexpr char const *LOGGING_NAME = "MuddleLearnerNetworkerImpl";

  explicit MuddleLearnerNetworkerImpl(const std::string &priv, unsigned short int port,
                                      const std::string &remote = "");

  explicit MuddleLearnerNetworkerImpl(MuddlePtr mud, StorePtr update_store);

  explicit MuddleLearnerNetworkerImpl(fetch::json::JSONDocument &cloud_config,
                                      std::size_t                instance_number);

  ~MuddleLearnerNetworkerImpl() override;

  MuddleLearnerNetworkerImpl(MuddleLearnerNetworkerImpl const &other) = delete;
  MuddleLearnerNetworkerImpl &operator=(MuddleLearnerNetworkerImpl const &other)  = delete;
  bool                        operator==(MuddleLearnerNetworkerImpl const &other) = delete;
  bool                        operator<(MuddleLearnerNetworkerImpl const &other)  = delete;

  void PushUpdate(UpdatePtr const &update, AlgorithmClass const &algorithm,
                  UpdateClass const &upd_class) override
  {
    PushUpdate(update->data(), algorithm, upd_class);
  }

  void PushUpdate(Bytes const &update, AlgorithmClass const &algo_name,
                  UpdateClass const &upd_class) override
  {
    PushUpdateBytes(algo_name, upd_class, update);
  }

  std::size_t GetUpdateCount(AlgorithmClass const &algo = "algo0",
                             UpdateClass const &   type = "gradients") const override
  {
    return update_store_->GetUpdateCount(algo, type);
  }

  std::size_t GetUpdateTotalCount() const override
  {
    return update_store_->GetUpdateCount();
  }

  ConstUpdatePtr GetUpdate(AlgorithmClass const &algo = "algo0",
                           UpdateType const &    type = "gradients") override
  {
    return update_store_->GetUpdate(algo, type);
  }

  void PushUpdateBytes(AlgorithmClass const &algo_name, UpdateType const &type_name,
                       Bytes const &update);
  void PushUpdateBytes(AlgorithmClass const &algo_name, UpdateType const &type_name,
                       Bytes const &update, Peers const &peers);
  void PushUpdateBytes(AlgorithmClass const &algo_name, UpdateType const &type_name,
                       Bytes const &update, Peers const &peers, double broadcast_proportion);

  ConstUpdatePtr GetUpdate(AlgorithmClass const &algo, UpdateType const &type,
                           Criteria const &criteria);

  virtual void submit(TaskPtr const &t);

  // This is the exposed interface

  uint64_t NetworkColearnUpdate(service::CallContext const &context,
                                AlgorithmClass const &algo_name, const std::string &type_name,
                                byte_array::ConstByteArray bytes, double proportion = 1.0,
                                double random_factor = 0.0);

  Randomiser &access_randomiser()
  {
    return randomiser_;
  }

  void set_broadcast_proportion(double proportion)
  {
    broadcast_proportion_ = std::max(0.0, std::min(1.0, proportion));
  }

  Address     GetAddress() const;
  std::string GetAddressAsString() const;

  void SetShuffleAlgorithm(const std::shared_ptr<ShuffleAlgorithmInterface> &alg);

  std::size_t GetPeerCount() const
  {
    return supplied_peers_.size();
  }

  void AddPeers(const std::vector<std::string> &new_peers)
  {
    for (auto const &peer : new_peers)
    {
      supplied_peers_.emplace_back(peer);
    }
  }
  void ClearPeers()
  {
    supplied_peers_.clear();
  }

protected:
  friend class MuddleOutboundAnnounceTask;
  void     Setup(MuddlePtr mud, StorePtr update_store);
  uint64_t ProcessUpdate(AlgorithmClass const &algo_name, const std::string &type_name,
                         byte_array::ConstByteArray bytes, double proportion, double random_factor,
                         const std::string &source);
  void     Setup(std::string const &priv, unsigned short int port,
                 std::unordered_set<std::string> const &remotes);

private:
  std::shared_ptr<Taskpool>   taskpool_;
  std::shared_ptr<Threadpool> tasks_runners_;
  MuddlePtr                   mud_;
  RpcClientPtr                client_;
  RpcServerPtr                server_;
  ProtoPtr                    proto_;
  StorePtr                    update_store_;
  Randomiser                  randomiser_;
  double                      broadcast_proportion_;
  double                      randomising_offset_;
  SubscriptionPtr             subscription_;
  byte_array::ConstByteArray  public_key_;

  NetManPtr   netm_;
  Sources     detected_peers_;
  SourcesList supplied_peers_;

  const unsigned int INITIAL_PEERS_COUNT = 10;

  mutable Mutex mutex_;

  std::shared_ptr<ShuffleAlgorithmInterface> alg_;

  friend class MuddleMessageHandler;
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
