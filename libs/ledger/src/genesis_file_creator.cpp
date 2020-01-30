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

#include "chain/address.hpp"
#include "chain/constants.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/filesystem/read_file_contents.hpp"
#include "crypto/hash.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"
#include "json/document.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/wallet_record.hpp"
#include "ledger/consensus/consensus_interface.hpp"
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
#include <utility>

namespace fetch {
namespace ledger {
namespace {

using fetch::byte_array::ConstByteArray;
using fetch::json::JSONDocument;
using fetch::storage::ResourceAddress;
using fetch::variant::Variant;

constexpr uint64_t    TOTAL_SUPPLY   = 11529975750000000000ull;
constexpr uint64_t    FET_MULTIPLIER = 10000000000ull;
constexpr char const *LOGGING_NAME   = "GenesisFile";
constexpr int         VERSION        = 4;

enum class FileReadStatus
{
  SUCCESS,
  FAILURE,
  FILE_NOT_PRESENT
};

/**
 * Load a JSON from a given path
 *
 * @param document The output document to be parsed
 * @param file_path The path to be read
 * @return true if successful, otherwise false
 */
FileReadStatus ParseDocument(JSONDocument &document, ConstByteArray const &contents)
{
  FileReadStatus status{FileReadStatus::FAILURE};

  if (contents.empty())
  {
    status = FileReadStatus::FILE_NOT_PRESENT;
  }
  else
  {
    try
    {
      document.Parse(contents);

      status = FileReadStatus::SUCCESS;
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to parse input file: ", ex.what());
    }
  }

  return status;
}

}  // namespace

using ConsensusPtr = std::shared_ptr<fetch::ledger::ConsensusInterface>;

GenesisFileCreator::GenesisFileCreator(StorageUnitInterface &storage_unit,
                                       CertificatePtr certificate, std::string const &db_prefix)
  : certificate_{std::move(certificate)}
  , storage_unit_{storage_unit}
  , db_name_{db_prefix + "_genesis_block"}
{}

/**
 * Load a 'genesis file file' with a given name
 *
 * @param name The path to the file to be loaded
 */
GenesisFileCreator::Result GenesisFileCreator::LoadContents(ConstByteArray const &contents,
                                                            bool                  proof_of_stake,
                                                            ConsensusParameters & params)
{
  // Perform a check as to whether we have installed genesis before
  {
    genesis_store_.Load(db_name_ + ".db", db_name_ + ".state.db");

    if (genesis_store_.Get(storage::ResourceAddress("HEAD"), genesis_block_))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Restored Genesis. Block 0x", genesis_block_.hash.ToHex(),
                     " Merkle: 0x", genesis_block_.merkle_hash.ToHex());

      chain::SetGenesisDigest(genesis_block_.hash);
      chain::SetGenesisMerkleRoot(genesis_block_.merkle_hash);

      params.whitelist    = genesis_block_.block_entropy.qualified;
      params.cabinet_size = static_cast<uint16_t>(params.whitelist.size());

      return Result::LOADED_PREVIOUS_GENESIS;
    }
  }

  // Failed - clear any state.
  genesis_block_ = Block();

  // Reset storage unit
  storage_unit_.Reset();

  json::JSONDocument doc{};

  auto const status = ParseDocument(doc, contents);

  bool success{false};
  if ((!proof_of_stake) && (status == FileReadStatus::FILE_NOT_PRESENT))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Building default genesis configuration");

    // in the stand alone case it is fine to have an empty genesis file if that is required
    success = LoadState(Variant::Array(0));
  }
  else if (status == FileReadStatus::SUCCESS)
  {
    // check the version
    int        version{0};
    bool const is_correct_version =
        variant::Extract(doc.root(), "version", version) && (version == VERSION);

    if (is_correct_version)
    {
      success = true;

      // Note: consensus has to be loaded before the state since that generates the block
      ConsensusParameters const *consensus_params{nullptr};
      if (proof_of_stake)
      {
        if (!LoadConsensus(doc["consensus"], params))
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Failed to successfully load consensus configuration");
          return Result::FAILURE;
        }

        // update the pointer the state will need to be updated
        consensus_params = &params;
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "No consensus information inside genesis file");
      }

      success &= LoadState(doc["accounts"], consensus_params);
    }
    else
    {
      FETCH_LOG_CRITICAL(LOGGING_NAME, "Incorrect stake file version! Found: ", version,
                         ". Expected: ", VERSION);
    }
  }

  // if the process was successful then trigger the flushing of the genesis block to storage
  if (success)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Saving genesis block");

    genesis_store_.Set(storage::ResourceAddress("HEAD"), genesis_block_);
    genesis_store_.Flush(false);

    return Result::CREATED_NEW_GENESIS;
  }

  return Result::FAILURE;
}

/**
 * Restore state from an input variant object
 *
 * @param object The reference state to be restored
 */
bool GenesisFileCreator::LoadState(Variant const &object, ConsensusParameters const *consensus)
{
  // Expecting an array of record entries
  if (!object.IsArray())
  {
    return false;
  }

  // iterate over all of the Identity + stake amount mappings
  uint64_t remaining_supply{TOTAL_SUPPLY};
  for (std::size_t i = 0, end = object.size(); i < end; ++i)
  {
    chain::Address address{};
    ConstByteArray address_raw{};
    uint64_t       balance{0};
    uint64_t       stake{0};

    auto const &obj{object[i]};

    if (variant::Extract(obj, "address", address_raw) &&
        variant::Extract(obj, "balance", balance) && variant::Extract(obj, "stake", stake) &&
        chain::Address::Parse(address_raw, address))
    {
      ledger::WalletRecord record;

      // adjust record values to be correct FET integer ranges
      balance *= FET_MULTIPLIER;
      stake *= FET_MULTIPLIER;

      // check the remaining supply
      if (balance > remaining_supply)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Invalid genesis configuration");
        return false;
      }
      remaining_supply -= balance;

      if (stake > remaining_supply)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Invalid genesis configuration");
        return false;
      }
      remaining_supply -= stake;

      // populate the record
      record.balance = balance;
      record.stake   = stake;

      Variant v_deed;
      if (obj.Has("deed"))
      {
        if (!record.CreateDeed(obj["deed"]))
        {
          return false;
        }
      }

      ResourceAddress const wallet_key{"fetch.token.state." + address.display()};

      {
        // serialize the record to the buffer
        serializers::LargeObjectSerializeHelper buffer;
        buffer << record;

        // store the buffer
        storage_unit_.Set(wallet_key, buffer.data());
      }
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to extract section from genesis file");
      return false;
    }
  }

  // ensure all token supply is taken
  if (remaining_supply > 0)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Remaining token supply still available: ", remaining_supply);
    return false;
  }

  // if we have been configured for consensus then we need to also write the stake information to
  // the state database
  if (consensus != nullptr)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Generating initial stakers configuration");

    // build the initial stake manager configuration
    StakeManager stake_manager{};
    stake_manager.Reset(*(consensus->snapshot), consensus->cabinet_size);
    if (!stake_manager.Save(storage_unit_))
    {
      FETCH_LOG_CRITICAL(LOGGING_NAME, "Unable to save initial stake queue to DB");
      return false;
    }
  }

  // Commit this state
  auto merkle_commit_hash = storage_unit_.Commit(0);

  FETCH_LOG_INFO(LOGGING_NAME, "Committed genesis merkle hash: 0x", merkle_commit_hash.ToHex());

  genesis_block_.timestamp    = (consensus != nullptr) ? consensus->start_time : 0;
  genesis_block_.merkle_hash  = merkle_commit_hash;
  genesis_block_.block_number = 0;
  genesis_block_.UpdateDigest();

  FETCH_LOG_INFO(LOGGING_NAME, "Created genesis block hash: 0x", genesis_block_.hash.ToHex());

  chain::SetGenesisMerkleRoot(merkle_commit_hash);
  chain::SetGenesisDigest(genesis_block_.hash);

  if (!storage_unit_.RevertToHash(merkle_commit_hash, 0))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed test to revert to merkle root!");
  }

  return true;
}

bool GenesisFileCreator::LoadConsensus(Variant const &object, ConsensusParameters &params)
{
  // attempt to parse all the basic consensus parameters
  bool const parse_success = variant::Extract(object, "cabinetSize", params.cabinet_size) &&
                             variant::Extract(object, "startTime", params.start_time);

  if (!parse_success || !object.Has("stakers"))
  {
    return false;
  }

  Variant const &stake_array = object["stakers"];
  if (!stake_array.IsArray())
  {
    return false;
  }

  params.snapshot = std::make_shared<StakeSnapshot>();

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
      auto address  = chain::Address(identity);

      FETCH_LOG_INFO(LOGGING_NAME, "Restoring stake. Identity: ", identity.identifier().ToBase64(),
                     " (address): ", address.address().ToBase64(), " amount: ", amount);

      // The initial set of miners is stored in the genesis block
      genesis_block_.block_entropy.qualified.insert(identity.identifier());

      params.snapshot->UpdateStake(identity, amount);
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Unable to parse stake entry from genesis file");
      return false;
    }
  }

  params.whitelist = genesis_block_.block_entropy.qualified;

  return true;
}

}  // namespace ledger
}  // namespace fetch
