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

#include "ledger/storage_unit/storage_unit_client.hpp"
#include "ledger/chain/constants.hpp"

#include <algorithm>

namespace fetch {
namespace ledger {

using PendingConnectionCounter =
    network::AtomicInFlightCounter<network::AtomicCounterName::LOCAL_SERVICE_CONNECTIONS>;

class MuddleLaneConnectorWorker : public network::AtomicStateMachine<StorageUnitClient::State>
{
public:
  using Address         = StorageUnitClient::Address;
  using FutureTimepoint = StorageUnitClient::FutureTimepoint;
  using LaneIndex       = StorageUnitClient::LaneIndex;
  using Muddle          = StorageUnitClient::Muddle;
  using MuddlePtr       = std::shared_ptr<Muddle>;
  using Peer            = fetch::network::Peer;
  using Promise         = StorageUnitClient::Promise;
  using PromiseState    = StorageUnitClient::PromiseState;
  using Uri             = StorageUnitClient::Uri;
  using State           = StorageUnitClient::State;
  using Client          = StorageUnitClient::Client;

  LaneIndex lane;
  Promise   lane_prom;
  Promise   count_prom;
  Address   target_address;

  MuddleLaneConnectorWorker(LaneIndex thelane, std::string thename, Uri thepeer,
                            MuddlePtr                 themuddle,
                            std::chrono::milliseconds thetimeout = std::chrono::milliseconds(10000))
    : lane(thelane)
    , name_(std::move(thename))
    , peer_(std::move(thepeer))
    , timeduration_(std::move(thetimeout))
    , muddle_(std::move(themuddle))
  {
    client_ = std::make_shared<Client>("R:MLC-" + std::to_string(lane), muddle_->AsEndpoint(),
                                       Muddle::Address(), SERVICE_LANE, CHANNEL_RPC);

    this->Allow(State::CONNECTING, State::INITIAL)
        .Allow(State::QUERYING, State::CONNECTING)
        .Allow(State::SUCCESS, State::QUERYING)

        .Allow(State::TIMEDOUT, State::INITIAL)
        .Allow(State::TIMEDOUT, State::CONNECTING)
        .Allow(State::TIMEDOUT, State::QUERYING)

        .Allow(State::FAILED, State::CONNECTING)
        .Allow(State::FAILED, State::QUERYING)
        .Allow(State::FAILED, State::INITIAL);
  }
  static constexpr char const *LOGGING_NAME = "MuddleLaneConnectorWorker";

  virtual ~MuddleLaneConnectorWorker()
  {
    counter_.Completed();
  }

  PromiseState Work()
  {
    try
    {
      this->AtomicStateMachine::Work();
    }
    catch (...)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "MuddleLaneConnectorWorker::Work lane ", lane, " threw.");
      throw;
    }
    auto r = this->AtomicStateMachine::Get();

    switch (r)
    {
    case State::TIMEDOUT:
      return PromiseState::TIMEDOUT;
    case State::SUCCESS:
      return PromiseState::SUCCESS;
    case State::FAILED:
      return PromiseState::FAILED;
    default:
      return PromiseState::WAITING;
    }
  }

  virtual bool PossibleNewState(State &currentstate) override
  {
    if (currentstate == State::TIMEDOUT || currentstate == State::SUCCESS ||
        currentstate == State::FAILED)
    {
      return false;
    }

    if (currentstate == State::INITIAL)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Timeout for lane ", lane, " set ", timeduration_.count());
      timeout_.Set(timeduration_);
    }

    if (timeout_.IsDue())
    {
      currentstate = State::TIMEDOUT;
      FETCH_LOG_INFO(LOGGING_NAME, "Timeout for lane ", lane);
      return true;
    }

    switch (currentstate)
    {
    case State::INITIAL:
    {
      FETCH_LOG_INFO(LOGGING_NAME, "INITIAL lane ", lane);
      muddle_->AddPeer(peer_);
      currentstate = State::CONNECTING;
      return true;
    }
    case State::CONNECTING:
    {
      bool connected = muddle_->UriToDirectAddress(peer_, target_address);
      if (!connected)
      {
        return false;
      }
      currentstate = State::QUERYING;
      lane_prom    = client_->CallSpecificAddress(target_address, RPC_IDENTITY,
                                               LaneIdentityProtocol::GET_LANE_NUMBER);
      count_prom   = client_->CallSpecificAddress(target_address, RPC_IDENTITY,
                                                LaneIdentityProtocol::GET_TOTAL_LANES);

      currentstate = State::QUERYING;
      return true;
    }
    case State::QUERYING:
    {
      auto p1 = count_prom->GetState();
      auto p2 = lane_prom->GetState();

      if ((p1 == PromiseState::FAILED) || (p2 == PromiseState::FAILED))
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Querying failed for lane ", lane);
        currentstate = State::FAILED;
        return true;
      }

      if ((p1 == PromiseState::WAITING) || (p2 == PromiseState::WAITING))
      {
        FETCH_LOG_INFO(LOGGING_NAME, "WAITING lane ", lane);
        return false;
      }

      currentstate = State::SUCCESS;
      return true;
    }
    default:
      FETCH_LOG_INFO(LOGGING_NAME, "Defaulted to fail for lane ", lane);
      currentstate = State::FAILED;
      return true;
    }
  }

private:
  std::shared_ptr<Client>   client_;
  std::string               name_;
  Uri                       peer_;
  std::chrono::milliseconds timeduration_;
  PendingConnectionCounter  counter_;
  MuddlePtr                 muddle_;
  FutureTimepoint           timeout_;
};

void StorageUnitClient::WorkCycle()
{
  if (!bg_work_.WorkCycle())
  {
    return;
  }

  if (bg_work_.CountSuccesses() > 0)
  {
    LaneIndex total_lanecount = 0;

    FETCH_LOG_INFO(LOGGING_NAME, " PEND=", bg_work_.CountPending());
    FETCH_LOG_INFO(LOGGING_NAME, " SUCC=", bg_work_.CountSuccesses());
    FETCH_LOG_INFO(LOGGING_NAME, " FAIL=", bg_work_.CountFailures());
    FETCH_LOG_INFO(LOGGING_NAME, " TOUT=", bg_work_.CountTimeouts());

    for (auto &successful_worker : bg_work_.GetSuccesses(1000))
    {
      if (successful_worker)
      {
        auto lanenum = successful_worker->lane;
        FETCH_LOG_VARIABLE(lanenum);
        FETCH_LOG_INFO(LOGGING_NAME, " PROCESSING lane ", lanenum);

        LaneIndex        lane;
        LaneIndex        total_lanes;
        crypto::Identity lane_identity;

        successful_worker->lane_prom->As(lane);
        successful_worker->count_prom->As(total_lanes);

        // TODO(issue 24): Verify expected identity

        {
          FETCH_LOCK(mutex_);
          lane_to_identity_map_[lane] = std::move(successful_worker->target_address);

          if (!total_lanecount)
          {
            total_lanecount = total_lanes;
            SetLaneLog2(uint32_t(total_lanecount));
          }
          else
          {
            assert(total_lanes == total_lanecount);
          }
        }
      }
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Lanes Connected   :", lane_to_identity_map_.size());
    FETCH_LOG_INFO(LOGGING_NAME, "log2_lanes_       :", log2_lanes_);
    FETCH_LOG_INFO(LOGGING_NAME, "total_lanecount   :", total_lanecount);
  }

  bg_work_.DiscardFailures();
  bg_work_.DiscardTimeouts();
}

void StorageUnitClient::AddLaneConnections(const std::map<LaneIndex, Uri> & lanes,
                                           const std::chrono::milliseconds &timeout)
{
  if (!workthread_)
  {
    workthread_ = std::make_shared<BackgroundedWorkThread>(&bg_work_, "BW:StoreUC",
                                                           [this]() { this->WorkCycle(); });
  }

  // number of lanes is the number of the last lane asked for +1
  LaneIndex m = lanes.empty() ? 1 : lanes.rbegin()->first + 1;

  if (m > muddles_.size())
  {
    SetNumberOfLanes(m);
  }

  for (auto const &lane : lanes)
  {
    auto        lanenum = lane.first;
    auto        peer    = lane.second;
    std::string name    = peer.ToString();

    auto worker = std::make_shared<MuddleLaneConnectorWorker>(
        lanenum, name, peer, GetMuddleForLane(lanenum), std::chrono::milliseconds(timeout));
    bg_work_.Add(worker);
  }
}

size_t StorageUnitClient::AddLaneConnectionsWaiting(const std::map<LaneIndex, Uri> & lanes,
                                                    const std::chrono::milliseconds &timeout)
{
  AddLaneConnections(lanes, timeout);
  PendingConnectionCounter::Wait(FutureTimepoint(timeout));

  std::size_t nLanes;
  {
    FETCH_LOCK(mutex_);
    nLanes = lane_to_identity_map_.size();
  }
  FETCH_LOG_INFO(LOGGING_NAME, "Successfully connected ", nLanes, " lane(s).");
  return nLanes;
}

// Convenience function for repeated code below
void GetClientAddress(StorageUnitClient::LaneIndex lane, StorageUnitClient::Address address,
                      StorageUnitClient *this_ptr)
{
  try
  {
    this_ptr->GetAddressForLane(lane, address);
  }
  catch (std::runtime_error &e)
  {
    throw e;
  }
}

// Get the current hash of the world state (merkle tree root)
byte_array::ConstByteArray StorageUnitClient::CurrentHash()
{
  MerkleTree                    tree{1u << log2_lanes_};
  std::vector<service::Promise> promises;

  for (auto const &lanedata : lane_to_identity_map_)
  {
    auto const &address = lanedata.second;
    auto        client  = GetClientForLane(lanedata.first);
    auto        promise = client->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::CURRENT_HASH);
    promises.push_back(promise);
  }

  std::size_t index = 0;
  for (auto &p : promises)
  {
    FETCH_LOG_PROMISE();
    tree[index] = p->As<byte_array::ByteArray>();
    index++;
  }

  tree.CalculateRoot();
  return tree.root();
}

// return the last committed hash (should correspond to the state hash before you began execution)
byte_array::ConstByteArray StorageUnitClient::LastCommitHash()
{
  FETCH_LOCK(merkle_mutex_);
  return current_merkle_->root();
}

// Revert to a previous hash if possible
bool StorageUnitClient::RevertToHash(Hash const &hash)
{
  // determine if the unit requests the genesis block
  bool const genesis_state = hash == GENESIS_MERKLE_ROOT;

  MerkleTreePtr tree;
  if (genesis_state)
  {
    // create the new tree
    tree = std::make_shared<MerkleTree>(1u << log2_lanes_);

    // fill the tree with empty leaf nodes
    for (std::size_t i = 0, num_lanes = 1u << log2_lanes_; i < num_lanes; ++i)
    {
      (*tree)[i] = GENESIS_MERKLE_ROOT;
    }
  }
  else
  {
    // Try to find whether we believe the hash exists (look in memory)
    // TODO(HUT): this is going to be tricky if the previous state has less/more lanes

    FETCH_LOCK(merkle_mutex_);

    auto it = std::find_if(state_merkle_stack_.rbegin(), state_merkle_stack_.rend(),
                           [&hash](MerkleTreePtr const &ptr) { return ptr->root() == hash; });

    if (state_merkle_stack_.rend() != it)
    {
      // update the tree
      tree = *it;

      // remove the other commits that occured later than this stack
      state_merkle_stack_.erase(it.base(), state_merkle_stack_.end());
    }
  }

  if (!tree)
  {
    FETCH_LOG_ERROR(LOGGING_NAME,
                    "Unable to lookup the required commit hash: ", byte_array::ToBase64(hash));
    return false;
  }

  // Note: we shouldn't be touching the lanes at this point from other threads
  std::vector<service::Promise> promises;

  // the check to see if the hash exists is only necessary for non genesis blocks since we know that
  // we can always revert back to nothing!
  if (!genesis_state)
  {
    // Due diligence: check all lanes can revert to this state before trying this operation
    StorageUnitClient::LaneIndex lane_index{0};
    for (auto const &hash : *tree)
    {
      assert(hash.size() > 0);

      Address address;
      GetClientAddress(lane_index, address, this);

      auto client = GetClientForLane(lane_index);

      auto promise = client->CallSpecificAddress(
          address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::HASH_EXISTS, hash);
      promises.push_back(promise);

      ++lane_index;
    }

    std::vector<uint32_t> failed_lanes;
    uint32_t              lane_counter = 0;

    for (auto &p : promises)
    {
      FETCH_LOG_PROMISE();

      if (!p->As<bool>())
      {
        failed_lanes.push_back(lane_counter);
      }

      lane_counter++;
    }

    for (auto const &i : failed_lanes)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Merkle hash mismatch: hash not found in lane: ", i);
      return false;
    }

    promises.clear();
  }

  // Now perform the revert
  StorageUnitClient::LaneIndex lane_index{0};
  for (auto const &hash : *tree)
  {
    assert(hash.size() > 0);

    Address address;
    GetClientAddress(lane_index, address, this);

    auto client = GetClientForLane(lane_index);

    auto promise = client->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::REVERT_TO_HASH, hash);
    promises.push_back(promise);
  }

  for (auto &p : promises)
  {
    FETCH_LOG_PROMISE();
    if (!p->As<bool>())
    {
      // TODO(HUT): revert all lanes to their previous state
      throw std::runtime_error(
          "Failed to revert all of the lanes -\
          the system may have entered an unknown state");
      return false;
    }
  }

  // since the state has now been restored we can update the current merkle reference
  {
    FETCH_LOCK(merkle_mutex_);
    current_merkle_ = tree;
  }

  return true;
}

// We have finished execution presumably, commit this state
byte_array::ConstByteArray StorageUnitClient::Commit()
{
  MerkleTreePtr                 tree = std::make_shared<MerkleTree>(1u << log2_lanes_);
  std::vector<service::Promise> promises;

  for (auto const &lanedata : lane_to_identity_map_)
  {
    auto const &address = lanedata.second;
    auto        client  = GetClientForLane(lanedata.first);
    auto        promise = client->CallSpecificAddress(
        address, RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::COMMIT);
    promises.push_back(promise);
  }

  std::size_t index = 0;
  for (auto &p : promises)
  {
    FETCH_LOG_PROMISE();
    (*tree)[index] = p->As<byte_array::ByteArray>();
    ++index;
  }

  tree->CalculateRoot();

  auto const tree_root = tree->root();

  {
    FETCH_LOCK(merkle_mutex_);
    current_merkle_ = tree;

    // this is a little
    if (!HashInStack(tree_root))
    {
      state_merkle_stack_.push_back(tree);
    }
  }

  return tree_root;
}

bool StorageUnitClient::HashExists(Hash const &hash)
{
  FETCH_LOCK(merkle_mutex_);
  return HashInStack(hash);
}

bool StorageUnitClient::HashInStack(Hash const &hash)
{
  // small optimisation to do the search in reverse
  auto const it = std::find_if(state_merkle_stack_.crbegin(), state_merkle_stack_.crend(),
                               [&hash](MerkleTreePtr const &ptr) { return ptr->root() == hash; });

  return state_merkle_stack_.rend() == it;
}

}  // namespace ledger
}  // namespace fetch
