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
#include "ledger/genesis_loading/genesis_file_creator.hpp"

#include "core/byte_array/decoders.hpp"
#include "core/json/document.hpp"

#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

namespace fetch {

GenesisFileCreator::GenesisFileCreator(BlockCoordinator &    block_coordinator,
                                       StorageUnitInterface &storage_unit)
  : block_coordinator_{block_coordinator}
  , storage_unit_{storage_unit}
{}

// Create a 'state file' with a given name
void GenesisFileCreator::CreateFile(std::string const &name)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Getting keys from state database");

  // Get all of the keys from the state database
  std::vector<ResourceID> keys = storage_unit_.KeyDump();

  FETCH_LOG_INFO(LOGGING_NAME, "Got: ", keys.size(), " keys.");

  Variant json = Variant::Object();

  for (auto const &key : keys)
  {
    auto corresponding_object_document = storage_unit_.Get(ResourceAddress(key));

    if (corresponding_object_document.failed)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to get a valid key value pair from the state database");
    }

    auto key_as_b64   = key.id().ToBase64();
    auto value_as_b64 = corresponding_object_document.document.ToBase64();

    json[key_as_b64] = value_as_b64;
  }

  {
    std::stringstream res;
    res << json;

    std::ofstream myfile;
    myfile.open(name);
    myfile << res.str();
    myfile.close();
  }
}

// Load a 'state file' with a given name
void GenesisFileCreator::LoadFile(std::string const &name)
{
  std::streampos size;
  ByteArray      array;
  std::ifstream  file(name, std::ios::in | std::ios::binary | std::ios::ate);

  if (file.is_open())
  {
    size = file.tellg();

    if (size == 0)
    {
      return;
    }

    array.Resize(std::size_t(size));
    file.seekg(0, std::ios::beg);
    file.read(array.char_pointer(), size);
    file.close();

    FETCH_LOG_DEBUG(LOGGING_NAME, "parsed: ", array);
  }
  else
  {
    return;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Clearing state and installing genesis");

  // Reset storage unit
  storage_unit_.Reset();

  json::JSONDocument doc(array);
  Variant            root = doc.root();

  // Set our state
  root.IterateObject([this](ConstByteArray const &key, variant::Variant const &value) {
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

}  // namespace fetch
