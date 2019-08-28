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

#include "core/byte_array/decoders.hpp"
#include "core/json/document.hpp"
#include "crypto/hash.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chain/constants.hpp"
#include "ledger/chaincode/wallet_record.hpp"
#include "ledger/consensus/stake_manager.hpp"
#include "ledger/consensus/stake_snapshot.hpp"
#include "ledger/genesis_loading/genesis_file_creator.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "storage/resource_mapper.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>

namespace fetch {
namespace ledger {
namespace {

using fetch::storage::ResourceAddress;
using fetch::storage::ResourceID;
using fetch::variant::Variant;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;
using fetch::json::JSONDocument;

constexpr char const *LOGGING_NAME = "GenesisFile";
constexpr int         VERSION      = 2;

/**
 * Load the entire file into a buffer
 *
 * @param file_path The path of the file to be loaded
 * @return The buffer of data
 */
ConstByteArray LoadFileContents(std::string const &file_path)
{
  std::streampos size;
  ByteArray      buffer;
  std::ifstream  file(file_path, std::ios::in | std::ios::binary | std::ios::ate);

  if (file.is_open())
  {
    size = file.tellg();

    if (size == 0)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to load stakefile! : ", file_path);
    }

    if (size > 0)
    {
      buffer.Resize(static_cast<std::size_t>(size));

      file.seekg(0, std::ios::beg);
      file.read(buffer.char_pointer(), size);

      file.close();
    }
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to load stakefile! : ", file_path);
  }

  return {buffer};
}

/**
 * Load a JSON from a given path
 *
 * @param document The output document to be parsed
 * @param file_path The path to be read
 * @return true if successful, otherwise false
 */
bool LoadFromFile(JSONDocument &document, std::string const &file_path)
{
  bool success{false};

  auto const buffer = LoadFileContents(file_path);
  if (!buffer.empty())
  {
    try
    {
      document.Parse(buffer);

      success = true;
    }
    catch (std::runtime_error const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to parse input file: ", ex.what());
    }

    success = true;
  }

  return success;
}

}  // namespace

/**
 * Load a 'state file' with a given name
 *
 * @param name THe path to the file to be loaded
 */
void GenesisFileCreator::LoadFile(std::string const &name)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Clearing state and installing genesis");

  json::JSONDocument doc{};
  if (LoadFromFile(doc, name))
  {
    // check the version
    int        version{0};
    bool const is_correct_version =
        variant::Extract(doc.root(), "version", version) && (version == VERSION);

    if (is_correct_version)
    {
      LoadState(doc["accounts"]);

      if (stake_)
      {
        LoadStake(doc["stake"]);
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "No stake manager provided when loading from stake file!");
      }
    }
    else
    {
      FETCH_LOG_CRITICAL(LOGGING_NAME, "Incorrect stake file version!");
    }
  }
}

/**
 * Restore state from an input variant object
 *
 * @param object The reference state to be restored
 */
void GenesisFileCreator::LoadState(Variant const &object)
{
  // Reset storage unit
  storage_unit_.Reset();

  // Expecting an array of record entries
  if (!object.IsArray())
  {
    return;
  }

  // iterate over all of the Identity + stake amount mappings
  for (std::size_t i = 0, end = object.size(); i < end; ++i)
  {
    ConstByteArray key{};
    uint64_t       balance{0};
    uint64_t       stake{0};

    if (variant::Extract(object[i], "key", key) &&
        variant::Extract(object[i], "balance", balance) &&
        variant::Extract(object[i], "stake", stake))
    {
      ledger::WalletRecord record;

      record.balance = balance;
      record.stake   = stake;

      ResourceAddress key_raw(ResourceID(FromBase64(key)));

      FETCH_LOG_INFO(LOGGING_NAME, "Initial state entry: ", key, " balance: ", balance,
                     " stake: ", stake);

      {
        // serialize the record to the buffer
        serializers::MsgPackSerializer buffer;
        buffer << record;

        // lookup reference to the underlying buffer
        auto const &data = buffer.data();

        // store the buffer
        storage_unit_.Set(key_raw, data);
      }
    }
    else
    {
      return;
    }
  }

  // Commit this state
  auto merkle_commit_hash = storage_unit_.Commit(0);

  FETCH_LOG_INFO(LOGGING_NAME, "Committed genesis merkle hash: ", merkle_commit_hash.ToBase64());

  ledger::Block genesis_block;

  genesis_block.body.merkle_hash  = merkle_commit_hash;
  genesis_block.body.block_number = 0;
  genesis_block.body.miner        = ledger::Address(crypto::Hash<crypto::SHA256>(""));
  genesis_block.UpdateDigest();

  FETCH_LOG_INFO(LOGGING_NAME, "Created genesis block hash: ", genesis_block.body.hash.ToBase64());

  ledger::GENESIS_MERKLE_ROOT = merkle_commit_hash;
  ledger::GENESIS_DIGEST      = genesis_block.body.hash;

  block_coordinator_.Reset();
}

void GenesisFileCreator::LoadStake(Variant const &object)
{
  if (stake_)
  {
    if (!object.Has("stakers"))
    {
      return;
    }

    Variant const &stake_array = object["stakers"];
    if (!stake_array.IsArray())
    {
      return;
    }

    auto snapshot = std::make_shared<StakeSnapshot>();

    // iterate over all of the Identity + stake amount mappings
    for (std::size_t i = 0, end = stake_array.size(); i < end; ++i)
    {
      ConstByteArray identity_raw{};
      uint64_t       amount{0};

      if (variant::Extract(stake_array[i], "identity", identity_raw) &&
          variant::Extract(stake_array[i], "amount", amount))
      {

        FETCH_LOG_INFO(LOGGING_NAME, "Found identity raw!, ", identity_raw);

        auto identity = crypto::Identity(FromBase64(identity_raw));
        auto address  = Address(identity);

        FETCH_LOG_INFO(LOGGING_NAME,
                       "Restoring stake. Identity: ", identity.identifier().ToBase64(),
                       " (address): ", address.address().ToBase64(), " amount: ", amount);

        snapshot->UpdateStake(identity, amount);
      }
    }

    stake_->Reset(*snapshot);
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "No stake manager!");
  }
}

}  // namespace ledger
}  // namespace fetch
