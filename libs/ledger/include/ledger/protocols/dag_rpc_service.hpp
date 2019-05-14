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

#include "crypto/ecdsa.hpp"
#include "ledger/dag/dag.hpp"
#include "ledger/dag/dag_muddle_configuration.hpp"
#include "ledger/dag/dag_node.hpp"
#include "ledger/protocols/dag_rpc_protocol.hpp"
#include "network/management/network_manager.hpp"

#include "crypto/fnv.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/packet.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/service/protocol.hpp"

#include "core/byte_array/encoders.hpp"
#include "variant/variant.hpp"

#include <atomic>
#include <deque>
#include <memory>

namespace fetch {
namespace ledger {

class DAGRpcService : public muddle::rpc::Server, public std::enable_shared_from_this<DAGRpcService>
{
public:
  static constexpr char const *LOGGING_NAME = "DAGRpcService";

  enum class State
  {
    WAIT_FOR_NODES,
    ADD_URGENT_NODES,
    ADD_BACKLOG,
    ADD_NEW_NODES
  };

  static char const *ToString(State state)
  {
    switch (state)
    {
    case State::WAIT_FOR_NODES:
      return "WAIT_FOR_NODES";
    case State::ADD_URGENT_NODES:
      return "ADD_URGENT_NODES";
    case State::ADD_BACKLOG:
      return "ADD_BACKLOG";
    case State::ADD_NEW_NODES:
      return "ADD_NEW_NODES";
    }
    return "UNKNOWN";
  }

  enum  // TODO Move common place
  {
    DAG_CHUNK_SIZE = 100000
  };

  struct QueueItem
  {
    DAGNode node;
    int     attempts = 0;

    // TODO: add from adddress such that trust system can penalise bad behaviour
  };

  using MuddleEndpoint       = muddle::MuddleEndpoint;
  using Muddle               = muddle::Muddle;
  using SubscriptionPtr      = muddle::MuddleEndpoint::SubscriptionPtr;
  using VerifierType         = crypto::ECDSAVerifier;
  using ThreadPool           = network::ThreadPool;
  using NodeList             = std::vector<DAGNode>;
  using NodeQueue            = std::deque<QueueItem>;
  using Packet               = fetch::muddle::Packet;
  using DAGNodeAddedCallback = std::function<void(DAGNode const &)>;
  using StateMachine         = core::StateMachine<State>;
  using StateMachinePtr      = std::shared_ptr<StateMachine>;

  /// Service functions
  /// @{
  State IdleUntilWork();
  State AddUrgentNodes();
  State AddBacklog();
  State AddNewNodes();
  /// }

  // Construction / Destruction
  DAGRpcService(Muddle &muddle, MuddleEndpoint &endpoint, DAG &dag);
  DAGRpcService(DAGRpcService const &) = delete;
  DAGRpcService(DAGRpcService &&)      = delete;
  ~DAGRpcService()
  {
    //state_machine_->Reset();
    //state_machine_.reset();
  }

  DAGRpcService &operator=(DAGRpcService const &) = delete;
  DAGRpcService &operator=(DAGRpcService &&) = delete;

  void BroadcastDAGNode(DAGNode node);

  void SetCertificate(byte_array::ConstByteArray const &private_key)
  {
    LOG_STACK_TRACE_POINT;
    certificate_ = std::make_shared<fetch::crypto::ECDSASigner>();
    certificate_->Load(private_key);
  }

  void SignalNewDAGNode(DAGNode node, bool broadcast = true)
  {
    LOG_STACK_TRACE_POINT;
    if (!dag_.HasNode(node.hash))
    {
      dag_.Push(node);
      if (broadcast)
      {
        BroadcastDAGNode(node);
      }
    }
  }

private:
  using SignerPtr      = std::shared_ptr<crypto::ECDSASigner>;
  using RpcServerPtr   = std::unique_ptr<muddle::rpc::Server>;
  using DAGProtocolPtr = std::unique_ptr<DAGProtocol>;
  using ClientPtr      = std::shared_ptr<muddle::rpc::Client>;
  using Flag           = std::atomic<bool>;

  bool AddNodeToQueue(DAGNode node);

  DAGNode UnpackNode(byte_array::ConstByteArray const &msg)
  {
    LOG_STACK_TRACE_POINT;
    DAGNode ret;

    serializers::TypedByteArrayBuffer buf(msg);
    buf >> ret;

    return ret;
  }

  mutable fetch::mutex::Mutex global_mutex_{__LINE__, __FILE__};
  mutable fetch::mutex::Mutex urgent_queue_mutex_{__LINE__, __FILE__};
  NodeQueue                   urgent_node_queue_;

  mutable fetch::mutex::Mutex backlog_queue_mutex_{__LINE__, __FILE__};
  NodeQueue                   backlog_node_queue_;

  mutable fetch::mutex::Mutex normal_queue_mutex_{__LINE__, __FILE__};
  NodeQueue                   normal_node_queue_;

  Flag syncronising_{false};

  StateMachinePtr  state_machine_;  ///< The syncronisation state machine
  crypto::Identity identity_;
  RpcServerPtr     server_;
  DAGProtocolPtr   protocol_;
  SignerPtr        certificate_;
  ThreadPool       thread_pool_;

  /// @name DAG
  /// @{
  Muddle &        muddle_;
  MuddleEndpoint &endpoint_;
  DAG &           dag_;
  DAGProtocol     dag_protocol_;

  SubscriptionPtr      dag_subscription_;
  DAGNodeAddedCallback on_dag_node_added_;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
