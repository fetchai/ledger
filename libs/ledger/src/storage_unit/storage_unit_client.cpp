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
#include "ledger/chain/v2/transaction.hpp"
#include "ledger/chain/v2/transaction_layout_rpc_serializers.hpp"
#include "ledger/chain/v2/transaction_rpc_serializers.hpp"
#include "ledger/storage_unit/transaction_finder_protocol.hpp"

using fetch::storage::ResourceID;
using fetch::storage::RevertibleDocumentStoreProtocol;
using fetch::muddle::MuddleEndpoint;
using fetch::service::Promise;
using fetch::byte_array::ToBase64;

namespace fetch {
namespace ledger {
namespace {

using AddressList = std::vector<MuddleEndpoint::Address>;

AddressList GenerateAddressList(ShardConfigs const &shards)
{
  AddressList addresses{};
  addresses.reserve(shards.size());

  for (auto const &shard : shards)
  {
    addresses.emplace_back(shard.internal_identity->identity().identifier());
  }

  return addresses;
}

}  // namespace

using TxStoreProtocol = fetch::storage::ObjectStoreProtocol<Transaction>;

StorageUnitClient::StorageUnitClient(MuddleEndpoint &muddle, ShardConfigs const &shards,
                                     uint32_t log2_num_lanes)
  : addresses_(GenerateAddressList(shards))
  , log2_num_lanes_(log2_num_lanes)
  , rpc_client_("STUC", muddle, MuddleEndpoint::Address{}, SERVICE_LANE_CTRL, CHANNEL_RPC)
  , current_merkle_{num_lanes()}
{
  if (num_lanes() != shards.size())
  {
    throw std::logic_error("Incorrect number of shard configs");
  }

  permanent_state_merkle_stack_.Load(MERKLE_FILENAME, true);
  FETCH_LOG_INFO(LOGGING_NAME,
                 "After recovery, size of merkle stack is: ", permanent_state_merkle_stack_.size());
}

// Get the current hash of the world state (merkle tree root)
byte_array::ConstByteArray StorageUnitClient::CurrentHash()
{
  MerkleTree                    tree{num_lanes()};
  std::vector<service::Promise> promises;

  for (uint32_t i = 0; i < num_lanes(); ++i)
  {
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(i), RPC_STATE,
                                                   RevertibleDocumentStoreProtocol::CURRENT_HASH);

    promises.push_back(promise);
  }

  std::size_t index = 0;
  for (auto &p : promises)
  {
    FETCH_LOG_PROMISE();
    tree[index] = p->As<byte_array::ByteArray>();

    FETCH_LOG_DEBUG(LOGGING_NAME, "Merkle Hash ", index, ": 0x", tree[index].ToHex());

    ++index;
  }

  tree.CalculateRoot();

  FETCH_LOG_DEBUG(LOGGING_NAME, "Merkle Final Hash: 0x", tree.root().ToHex());

  return tree.root();
}

// return the last committed hash (should correspond to the state hash before you began execution)
byte_array::ConstByteArray StorageUnitClient::LastCommitHash()
{
  ConstByteArray last_commit_hash = GENESIS_MERKLE_ROOT;

  {
    FETCH_LOCK(merkle_mutex_);

    auto stack_size = permanent_state_merkle_stack_.size();

    if (stack_size > 0)
    {
      return current_merkle_.root();
    }
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Last Commit Hash: 0x", last_commit_hash.ToHex());

  return last_commit_hash;
}

// Revert to a previous hash if possible. Passing the block index makes this much more efficient
bool StorageUnitClient::RevertToHash(Hash const &hash, uint64_t index)
{
  // determine if the unit requests the genesis block
  bool const genesis_state = hash == GENESIS_MERKLE_ROOT;

  FETCH_LOCK(merkle_mutex_);

  // Set merkle stack to this hash, get the tree
  MerkleTree tree{num_lanes()};
  if (genesis_state && (index == 0))  // this is truly the genesis block
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Reverting state to genesis.");

    // fill the tree with empty leaf nodes
    for (std::size_t i = 0; i < num_lanes(); ++i)
    {
      tree[i] = GENESIS_MERKLE_ROOT;
    }

    permanent_state_merkle_stack_.New(MERKLE_FILENAME);  // clear the stack
    permanent_state_merkle_stack_.Push(MerkleTreeBlock{tree});
  }
  else
  {
    // Try to find whether we believe the hash exists (index into merkle stack)
    MerkleTreeBlock merkle_block;
    uint64_t const  merkle_stack_size = permanent_state_merkle_stack_.size();

    if (index >= merkle_stack_size)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Unsuccessful attempt to revert to hash ahead in the stack! Stack size: ",
                     merkle_stack_size, " revert index: ", index);
      return false;
    }

    permanent_state_merkle_stack_.Get(index, merkle_block);
    tree = merkle_block.Extract(num_lanes());

    if (tree.root() != hash)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Index given for merkle hash didn't match merkle stack!");
      return false;
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "Successfully found merkle at: ", index);
  }  // End set merkle stack

  // Note: we shouldn't be touching the lanes at this point from other threads
  std::vector<service::Promise> promises;
  promises.reserve(num_lanes());

  // Now perform the revert
  StorageUnitClient::LaneIndex lane_index{0};
  for (auto const &lane_merkle_hash : tree)
  {
    assert(!hash.empty());

    // make the call to the RPC server
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(lane_index++), RPC_STATE,
                                                   RevertibleDocumentStoreProtocol::REVERT_TO_HASH,
                                                   lane_merkle_hash);

    // add the promise to the queue
    promises.emplace_back(std::move(promise));
  }

  lane_index = 0;
  bool all_success{true};
  for (auto &p : promises)
  {
    FETCH_LOG_PROMISE();

    if (!p->As<bool>())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to revert shard ", lane_index, " to ",
                     tree[lane_index].ToHex());

      all_success &= false;
    }
  }

  if (all_success)
  {
    // Trim the merkle stack down now that we have had success reverting the lanes
    while (permanent_state_merkle_stack_.size() != index + 1)
    {
      permanent_state_merkle_stack_.Pop();
    }

    permanent_state_merkle_stack_.Flush(false);

    // since the state has now been restored we can update the current merkle reference
    current_merkle_ = tree;
  }

  return all_success;
}

// We have finished execution presumably, commit this state
byte_array::ConstByteArray StorageUnitClient::Commit(uint64_t commit_index)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Committing: ", commit_index);

  MerkleTree tree{num_lanes()};

  std::vector<service::Promise> promises;
  promises.reserve(num_lanes());

  for (uint32_t lane_idx = 0; lane_idx < num_lanes(); ++lane_idx)
  {
    // make the request to the RPC server
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(lane_idx), RPC_STATE,
                                                   RevertibleDocumentStoreProtocol::COMMIT);

    // add the promise to the waiting queue
    promises.push_back(promise);
  }

  std::size_t index = 0;
  for (auto &p : promises)
  {
    FETCH_LOG_PROMISE();
    tree[index] = p->As<byte_array::ByteArray>();

    ++index;
  }

  tree.CalculateRoot();

  auto const tree_root = tree.root();

  {
    FETCH_LOCK(merkle_mutex_);
    current_merkle_ = tree;

    if (permanent_state_merkle_stack_.size() != commit_index)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Committing to an index where there is a mismatch to the merkle stack!");
    }

    while (permanent_state_merkle_stack_.size() != commit_index + 1)
    {
      permanent_state_merkle_stack_.Push(MerkleTreeBlock{});
    }

    permanent_state_merkle_stack_.Set(commit_index, MerkleTreeBlock{tree});
    permanent_state_merkle_stack_.Flush(false);
  }

  return tree_root;
}

bool StorageUnitClient::HashExists(Hash const &hash, uint64_t index)
{
  // FETCH_LOCK(merkle_mutex_);
  return HashInStack(hash, index);
}

// Search backwards through stack
// TODO(HUT): should be const correct
bool StorageUnitClient::HashInStack(Hash const &hash, uint64_t index)
{
  MerkleTreeBlock proxy;
  FETCH_LOCK(merkle_mutex_);
  uint64_t const merkle_stack_size = permanent_state_merkle_stack_.size();

  if (index >= merkle_stack_size)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Tried to find hash in merkle tree when index more than stack: ",
                   merkle_stack_size, " index: ", index);
    return false;
  }

  permanent_state_merkle_stack_.Get(index, proxy);
  MerkleTree deser = proxy.Extract(num_lanes());

  if (deser.root() == hash)
  {
    return true;
  }

  return false;
}

StorageUnitClient::Address const &StorageUnitClient::LookupAddress(ShardIndex shard) const
{
  return addresses_.at(shard);
}

StorageUnitClient::Address const &StorageUnitClient::LookupAddress(
    storage::ResourceID const &resource) const
{
  return LookupAddress(resource.lane(log2_num_lanes_));
}

void StorageUnitClient::AddTransaction(Transaction const &tx)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Adding tx: 0x", tx.digest().ToHex());

  try
  {
    ResourceID resource{tx.digest()};

    // make the RPC request
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(resource), RPC_TX_STORE,
                                                   TxStoreProtocol::SET, resource, tx);

    // wait the for the response
    FETCH_LOG_PROMISE();
    promise->Wait();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to add transaction: ", ex.what());
  }
}

StorageUnitClient::TxLayouts StorageUnitClient::PollRecentTx(uint32_t max_to_poll)
{
  std::vector<service::Promise> promises;
  TxLayouts                     layouts;

  FETCH_LOG_DEBUG(LOGGING_NAME, "Polling recent transactions from lanes");

  // Assume that the lanes are roughly balanced in terms of new TXs
  for (auto const &lane_address : addresses_)
  {
    auto promise =
        rpc_client_.CallSpecificAddress(lane_address, RPC_TX_STORE, TxStoreProtocol::GET_RECENT,
                                        uint32_t(max_to_poll / addresses_.size()));
    FETCH_LOG_PROMISE();
    promises.push_back(promise);
  }

  for (auto const &promise : promises)
  {
    auto txs = promise->As<TxLayouts>();

    layouts.insert(layouts.end(), std::make_move_iterator(txs.begin()),
                   std::make_move_iterator(txs.end()));
  }

  return layouts;
}

bool StorageUnitClient::GetTransaction(byte_array::ConstByteArray const &digest,
                                       Transaction &                 tx)
{
  bool success{false};

  try
  {
    ResourceID resource{digest};

    // make the request to the RPC server
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(resource), RPC_TX_STORE,
                                                   TxStoreProtocol::GET, resource);

    // wait for the response to be delivered
    tx = promise->As<Transaction>();

    success = true;
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to get transaction, because: ", e.what());
  }

  return success;
}

bool StorageUnitClient::HasTransaction(ConstByteArray const &digest)
{
  bool present{false};

  try
  {
    ResourceID resource{digest};

    // make the request to the RPC server
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(resource), RPC_TX_STORE,
                                                   TxStoreProtocol::HAS, resource);

    // wait for the response to be delivered
    present = promise->As<bool>();

    FETCH_LOG_DEBUG(LOGGING_NAME, "TX: ", ToBase64(digest), " Present: ", present);
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to check transaction existence, because: ", e.what(),
                   " tx: ", ToBase64(digest));
  }

  return present;
}

void StorageUnitClient::IssueCallForMissingTxs(DigestSet const &tx_set)
{
  std::map<Address, std::unordered_set<ResourceID>> lanes_of_interest;
  for (auto const &hash : tx_set)
  {
    ResourceID resource{hash};
    lanes_of_interest[LookupAddress(resource)].insert(std::move(resource));
  }

  for (auto const &lane_resources : lanes_of_interest)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Request sent ", lane_resources.second.size(), " resources");
    rpc_client_.CallSpecificAddress(lane_resources.first, RPC_MISSING_TX_FINDER,
                                    TxFinderProtocol::ISSUE_CALL_FOR_MISSING_TXS,
                                    lane_resources.second);
  }
}

StorageUnitClient::Document StorageUnitClient::GetOrCreate(ResourceAddress const &key)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "GetOrCreate: ", key.address());

  Document doc;

  try
  {
    // make the request to the RPC client
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(key), RPC_STATE,
                                                   RevertibleDocumentStoreProtocol::GET_OR_CREATE,
                                                   key.as_resource_id());

    // wait for the document to be returned
    doc = promise->As<Document>();
  }
  catch (std::runtime_error const &e)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to get or create document, because: ", e.what());
    doc.failed = true;
  }

  return doc;
}

StorageUnitClient::Document StorageUnitClient::Get(ResourceAddress const &key)
{
  Document doc;

  try
  {
    // make the request to the RPC server
    auto promise = rpc_client_.CallSpecificAddress(
        LookupAddress(key), RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET,
        key.as_resource_id());

    // wait for the document response
    doc = promise->As<Document>();
  }
  catch (std::runtime_error const &e)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to get document, because: ", e.what());

    // signal the failure
    doc.failed = true;
  }

  return doc;
}

void StorageUnitClient::Set(ResourceAddress const &key, StateValue const &value)
{
  FETCH_LOG_DEBUG(LOGGING_NAME, "Set: ", key.address(), " value: 0x", value.ToHex());

  try
  {
    // make the request to the RPC server
    auto promise = rpc_client_.CallSpecificAddress(
        LookupAddress(key), RPC_STATE, fetch::storage::RevertibleDocumentStoreProtocol::SET,
        key.as_resource_id(), value);

    FETCH_LOG_PROMISE();

    // wait for the response
    promise->Wait();
  }
  catch (std::runtime_error const &e)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to call SET (store document), because: ", e.what());
  }
}

bool StorageUnitClient::Lock(ShardIndex index)
{
  bool success{false};

  try
  {
    // make the request to the RPC server
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(index), RPC_STATE,
                                                   RevertibleDocumentStoreProtocol::LOCK);

    // wait for the promise
    success = promise->As<bool>();
  }
  catch (std::runtime_error const &e)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to call Lock, because: ", e.what());
  }

  return success;
}

bool StorageUnitClient::Unlock(ShardIndex index)
{
  bool success{false};

  try
  {
    // make the request to the RPC server
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(index), RPC_STATE,
                                                   RevertibleDocumentStoreProtocol::UNLOCK);

    // wait for the result
    success = promise->As<bool>();
  }
  catch (std::runtime_error const &e)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to call Unlock, because: ", e.what());
  }

  return success;
}

}  // namespace ledger
}  // namespace fetch
