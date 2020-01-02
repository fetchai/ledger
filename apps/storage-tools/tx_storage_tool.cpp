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

#include "tx_storage_tool.hpp"
#include "wait_for_connections.hpp"

#include "chain/transaction.hpp"
#include "chain/transaction_serializer.hpp"
#include "core/filesystem/read_file_contents.hpp"

#include <chrono>

namespace {

using namespace std::chrono_literals;

constexpr char const *LOGGING_NAME = "TxStorageTool";

using fetch::muddle::MuddleInterface;
using fetch::muddle::CreateMuddle;
using fetch::chain::Transaction;
using fetch::chain::TransactionSerializer;

using InitialPeers = MuddleInterface::Peers;

std::string GenerateTxFilename(fetch::Digest const &digest)
{
  std::string base = static_cast<std::string>(digest.ToHex());
  return "0x" + base + ".fetch.ai.tx";
}

}  // namespace

TxStorageTool::TxStorageTool(uint32_t log2_num_lanes)
  : log2_num_lanes_{log2_num_lanes}
{
  if (log2_num_lanes != 0)
  {
    throw std::runtime_error{"More than one lane unsupported at the moment"};
  }

  // start the network manager
  nm_.Start();

  // create the shard muddle muddle
  net_ = CreateMuddle("ISRD", nm_, "127.0.0.1");

  // generate the set of initial peers to connect to
  InitialPeers initial_peers{};
  for (uint32_t i = 0; i < num_lanes_; ++i)
  {
    initial_peers.emplace(std::string{"tcp://127.0.0.1:"} + std::to_string((i * 2) + 8011));
  }

  // start the network
  net_->Start(initial_peers, {0});

  // wait until we have established the connection to the remote peer
  if (!WaitForPeerConnections(*net_, initial_peers.size()))
  {
    throw std::runtime_error{"Unable to connect to peers requested"};
  }

  // create the storage client
  auto const peers = net_->GetDirectlyConnectedPeers();
  client_          = std::make_unique<TxStorageClient>(
      TxStorageClient::LaneAddresses{peers.begin(), peers.end()}, net_->GetEndpoint());

  FETCH_LOG_INFO(LOGGING_NAME, "Initialisation complete");
}

TxStorageTool::~TxStorageTool()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Tearing Down");

  // tear down
  net_->Stop();
  nm_.Stop();
}

int TxStorageTool::Run(fetch::DigestSet const &tx_to_get, FilenameSet const &txs_to_set)
{
  bool all_success{true};

  // download and requested transactions
  for (auto const &tx : tx_to_get)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Downloading: 0x", tx.ToHex(), "...");

    bool const success = Download(tx);
    if (!success)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Downloading: 0x", tx.ToHex(), "...FAILED");
    }

    all_success &= success;
  }

  // upload and requested transaction files
  for (auto const &file_path : txs_to_set)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Uploading: ", file_path, "...");

    bool const success = Upload(file_path);
    if (!success)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Uploading: ", file_path, "...FAILED");
    }

    all_success &= success;
  }

  return all_success ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool TxStorageTool::Download(fetch::Digest const &digest)
{
  bool success{false};

  if (client_->HasTransaction(digest))
  {
    Transaction tx{};

    if (client_->GetTransaction(digest, tx))
    {
      // serialise the transaction into bytes
      TransactionSerializer serialiser{};
      serialiser << tx;

      // flush the transaction to disk
      auto const    file_path = GenerateTxFilename(digest);
      std::ofstream file_stream{file_path.c_str(), std::ios::out | std::ios::binary};
      file_stream.write(serialiser.data().char_pointer(),
                        static_cast<std::streamsize>(serialiser.data().size()));

      success = true;
    }
  }

  return success;
}

bool TxStorageTool::Upload(std::string const &filename)
{
  // attempt to read the contents of the file
  auto contents = fetch::core::ReadContentsOfFile(filename.c_str());
  if (contents.empty())
  {
    return false;
  }

  // attempt to reconstitute the binary transaction
  Transaction tx{};
  try
  {
    TransactionSerializer serializer{contents};
    serializer >> tx;
  }
  catch (std::exception const &)
  {
    return false;
  }

  // submit the transaction to the node
  return client_->AddTransaction(tx);
}
