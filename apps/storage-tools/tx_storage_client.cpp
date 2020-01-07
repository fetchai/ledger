//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "tx_storage_client.hpp"

#include "chain/transaction.hpp"
#include "chain/transaction_rpc_serializers.hpp"
#include "core/digest.hpp"
#include "core/service_ids.hpp"
#include "ledger/storage_unit/transaction_finder_protocol.hpp"
#include "ledger/storage_unit/transaction_storage_protocol.hpp"
#include "logging/logging.hpp"
#include "storage/resource_mapper.hpp"
#include "vectorise/platform.hpp"

using fetch::Digest;
using fetch::DigestSet;
using fetch::storage::ResourceID;
using fetch::ledger::TransactionStorageProtocol;

static constexpr char const *LOGGING_NAME = "TxStorageClient";

TxStorageClient::TxStorageClient(LaneAddresses lane_addresses, MuddleEndpoint &endpoint)
  : lane_addresses_{std::move(lane_addresses)}
  , log2_num_lanes_{fetch::platform::ToLog2(static_cast<uint32_t>(lane_addresses.size()))}
  , rpc_client_{"StoreClient", endpoint, fetch::SERVICE_LANE_CTRL, fetch::CHANNEL_RPC}
{}

bool TxStorageClient::AddTransaction(Transaction const &tx)
{
  bool success{false};

  try
  {
    ResourceID resource{tx.digest()};

    // make the RPC request
    auto promise = rpc_client_.CallSpecificAddress(LookupAddress(resource), fetch::RPC_TX_STORE,
                                                   TransactionStorageProtocol::ADD, resource, tx);

    // wait the for the response
    promise->Wait();

    success = true;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to add transaction: ", ex.what());
  }

  return success;
}

bool TxStorageClient::GetTransaction(Digest const &digest, Transaction &tx)
{
  ResourceID const resource{digest};

  // make the request to the RPC server
  auto promise = rpc_client_.CallSpecificAddress(LookupAddress(resource), fetch::RPC_TX_STORE,
                                                 TransactionStorageProtocol::GET, resource);

  // wait for the response to be delivered
  bool const success = promise->GetResult(tx);
  if (!success)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup transaction 0x", digest.ToHex());
  }

  return success;
}

bool TxStorageClient::HasTransaction(Digest const &digest)
{
  ResourceID const resource{digest};

  // make the request to the RPC server
  auto promise = rpc_client_.CallSpecificAddress(LookupAddress(resource), fetch::RPC_TX_STORE,
                                                 TransactionStorageProtocol::HAS, resource);

  // wait for the response to be delivered
  bool present{false};
  return promise->GetResult(present) && present;
}

TxStorageClient::MuddleAddress const &TxStorageClient::LookupAddress(
    ResourceID const &resource) const
{
  return lane_addresses_.at(resource.lane(log2_num_lanes_));
}
