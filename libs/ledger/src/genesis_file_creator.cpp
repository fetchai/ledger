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

#include "ledger/chain/address.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/constants.hpp"
#include "storage/resource_mapper.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/genesis_loading/genesis_file_creator.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/consensus/stake_manager.hpp"
#include "ledger/consensus/stake_snapshot.hpp"

#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/json/document.hpp"

#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

#include <fstream>

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
constexpr int VERSION  = 1;

/**
 * Dump the contents of the variant as JSON to the target file
 *
 * @param data The variant to dump
 * @param file_path The file path to be created/updated
 */
void DumpToFile(Variant const &data, std::string const &file_path)
{
  // format the data
  std::ostringstream stream;
  stream << data;

  // close the
  std::ofstream genesis_file{};
  genesis_file.open(file_path);
  genesis_file << stream.str();
  genesis_file.close();
}

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

    if (size > 0)
    {
      buffer.Resize(static_cast<std::size_t>(size));

      file.seekg(0, std::ios::beg);
      file.read(buffer.char_pointer(), size);

      file.close();
    }
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

} // namespace


/**
 * Create a 'state file' with the specified name
 *
 * @param name The output file path to be populated
 */
void GenesisFileCreator::CreateFile(std::string const &name)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Getting keys from state database");

  Variant payload = Variant::Object();
  payload["version"] = VERSION;

  Variant &state = payload["state"] = Variant::Object();
  Variant &stake = payload["stake"] = Variant::Object();

  DumpState(state);

  if (stake_manager_)
  {
    DumpStake(stake);
  }

  // dump the keys
  DumpToFile(payload, name);
}

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
    int version{0};
    bool const is_correct_version = variant::Extract(doc.root(), "version", version) && (version == VERSION);

    if (is_correct_version)
    {
      LoadState(doc["state"]);

      if (stake_manager_)
      {
        LoadStake(doc["stake"]);
      }
    }
  }
}

/**
 * Generate a Variant object containing the state summary of the system
 *
 * @param object The object to be populated
 */
void GenesisFileCreator::DumpState(variant::Variant &object)
{
  // Get all of the keys from the state database
  std::vector<ResourceID> keys = storage_unit_.KeyDump();

  FETCH_LOG_INFO(LOGGING_NAME, "Got: ", keys.size(), " keys.");

  for (auto const &key : keys)
  {
    auto corresponding_object_document = storage_unit_.Get(ResourceAddress(key));

    if (corresponding_object_document.failed)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to get a valid key value pair from the state database");
    }

    auto key_as_b64   = key.id().ToBase64();
    auto value_as_b64 = corresponding_object_document.document.ToBase64();

    object[key_as_b64] = value_as_b64;
  }
}

/**
 * Dump the stake information to the section of the file
 *
 * @param object The output object to be populated
 */
void GenesisFileCreator::DumpStake(Variant &object)
{
  object = Variant::Object();

  if (stake_manager_)
  {
    // get the current stake snapshot
    auto const snapshot = stake_manager_->GetCurrentStakeSnapshot();

    object["committeeSize"] = stake_manager_->committee_size();

    Variant &stake_array = object["stakes"] = Variant::Array(snapshot->size());

    std::size_t idx{0};
    snapshot->IterateOver([&idx, &stake_array](Address const &address, uint64_t stake) {
      Variant &entry = stake_array[idx] = Variant::Object();

      entry["address"] = address.display();
      entry["amount"] = stake;
    });
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

  // Set our state
  object.IterateObject([this](ConstByteArray const &key, Variant const &value) {
    FETCH_LOG_DEBUG(LOGGING_NAME, "key ", key);
    FETCH_LOG_DEBUG(LOGGING_NAME, "value ", value);

    ResourceAddress key_raw(ResourceID(FromBase64(key)));
    ConstByteArray  value_raw(byte_array::FromBase64(value.As<ConstByteArray>()));

    storage_unit_.Set(key_raw, value_raw);

    return true;
  });

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
  if (stake_manager_)
  {
    std::size_t committee_size{1};

    if (!variant::Extract(object, "committeeSize", committee_size))
    {
      return;
    }

    if (!object.Has("stakes"))
    {
      return;
    }

    Variant const &stake_array = object["stakes"];
    if (!stake_array.IsArray())
    {
      return;
    }

    auto snapshot = std::make_shared<StakeSnapshot>();

    // iterate over all of the
    Address address;
    for (std::size_t i = 0, end = stake_array.size(); i < end; ++i)
    {
      ConstByteArray display_address{};
      uint64_t amount{0};

      if (variant::Extract(stake_array[i], "address", display_address) && variant::Extract(stake_array[i], "amount", amount))
      {
        if (Address::Parse(display_address, address))
        {
          FETCH_LOG_INFO(LOGGING_NAME, "Restoring stake. Address: ", display_address, " amount: ", amount);

          snapshot->UpdateStake(address, amount);
        }
      }
    }


  }
}

} // namespace ledger
}  // namespace fetch
