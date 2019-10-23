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

#include "chain/transaction.hpp"
#include "chain/transaction_builder.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/identity.hpp"
#include "ledger/chaincode/contract.hpp"
#include "ledger/fetch_msgpack.hpp"
#include "ledger/identifier.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "mock_storage_unit.hpp"

#include "gtest/gtest.h"

inline auto FullShards(std::size_t lane_count)
{
  fetch::BitVector retval{lane_count};
  retval.SetAllOne();
  return retval;
}

class ContractTest : public ::testing::Test
{
protected:
  using Identity              = fetch::crypto::Identity;
  using Identifier            = fetch::ledger::Identifier;
  using ConstByteArray        = fetch::byte_array::ConstByteArray;
  using ByteArray             = fetch::byte_array::ByteArray;
  using Contract              = fetch::ledger::Contract;
  using ContractPtr           = std::shared_ptr<Contract>;
  using StrictMockStorageUnit = testing::StrictMock<MockStorageUnit>;
  using MockStorageUnitPtr    = std::unique_ptr<StrictMockStorageUnit>;
  using Resources             = std::vector<ConstByteArray>;
  using CertificatePtr        = std::unique_ptr<fetch::crypto::ECDSASigner>;
  using AddressPtr            = std::unique_ptr<fetch::chain::Address>;
  using StateAdapter          = fetch::ledger::StateAdapter;
  using StateSentinelAdapter  = fetch::ledger::StateSentinelAdapter;
  using Query                 = Contract::Query;
  using IdentifierPtr         = std::shared_ptr<Identifier>;
  using CachedStorageAdapter  = fetch::ledger::CachedStorageAdapter;
  using TransactionPtr        = fetch::chain::TransactionBuilder::TransactionPtr;

  void SetUp() override
  {
    block_number_  = 0u;
    certificate_   = std::make_unique<fetch::crypto::ECDSASigner>();
    owner_address_ = std::make_unique<fetch::chain::Address>(certificate_->identity());
    storage_       = std::make_unique<StrictMockStorageUnit>();
  }

  class PayloadPacker
  {
  public:
    template <typename... Args>
    explicit PayloadPacker(Args... args)
    {
      // initialise the array
      packer_.pack_array(sizeof...(args));

      Pack(args...);
    }

    ConstByteArray GetBuffer() const
    {
      return ConstByteArray{reinterpret_cast<uint8_t const *>(buffer_.data()), buffer_.size()};
    }

  private:
    using Buffer = msgpack::sbuffer;
    using Packer = msgpack::packer<Buffer>;

    template <typename T, typename... Args>
    void Pack(T const &arg, Args... args)
    {
      // packet this argument
      PackInternal(arg);

      // pack the other arguments
      Pack(args...);
    }

    void Pack()
    {}

    template <typename T>
    void PackInternal(T const &arg)
    {
      packer_.pack(arg);
    }

    void PackInternal(fetch::chain::Address const &address)
    {
      auto const &id = address.address();

      // pack as an address
      packer_.pack_ext(id.size(), static_cast<int8_t>(0x4D));
      packer_.pack_ext_body(id.char_pointer(), static_cast<uint32_t>(id.size()));
    }

    Buffer buffer_{};
    Packer packer_{&buffer_};
  };

  template <typename... Args>
  Contract::Result SendSmartActionWithParams(ConstByteArray const &action, Args... args)
  {
    // pack all the data into a single payload
    PayloadPacker p{args...};

    return SendSmartAction(action, p.GetBuffer());
  }

  Contract::Result SendSmartAction(ConstByteArray const &action,
                                   ConstByteArray const &data = ConstByteArray{})
  {
    using fetch::chain::TransactionBuilder;
    using fetch::chain::Address;

    // build the transaction
    tx_ = TransactionBuilder()
              .From(Address{certificate_->identity()})
              .TargetSmartContract(*contract_address_, *owner_address_, shards_)
              .Action(action)
              .Signer(certificate_->identity())
              .Data(data)
              .Seal()
              .Sign(*certificate_)
              .Build();

    // adapt the storage engine for this execution
    StateSentinelAdapter storage_adapter{*storage_, *contract_name_, shards_};

    // dispatch the transaction to the contract
    contract_->Attach(storage_adapter);
    auto const status = contract_->DispatchTransaction(*tx_, block_number_++);
    contract_->Detach();

    return status;
  }

  Contract::Result SendAction(TransactionPtr const &tx)
  {
    using ContractMode = fetch::chain::Transaction::ContractMode;

    Identifier id;

    switch (tx->contract_mode())
    {
    case ContractMode::PRESENT:
      id = Identifier{tx->contract_digest().address().ToHex() + "." +
                      tx->contract_address().display()};
      break;
    case ContractMode::CHAIN_CODE:
      id = Identifier{tx->chain_code()};
      break;
    case ContractMode::NOT_PRESENT:
      throw std::runtime_error("Not implemented");
    case ContractMode::SYNERGETIC:
      throw std::runtime_error("Not implemented");
    }

    // adapt the storage engine for this execution
    StateSentinelAdapter storage_adapter{*storage_, std::move(id), shards_};

    // dispatch the transaction to the contract
    contract_->Attach(storage_adapter);
    auto const status = contract_->DispatchTransaction(*tx, block_number_++);
    contract_->Detach();

    return status;
  }

  Contract::Status SendQuery(ConstByteArray const &query, Query const &request, Query &response)
  {
    // adapt the storage engine for queries
    StateAdapter storage_adapter{*storage_, *contract_name_};

    // attach, dispatch and detach again
    contract_->Attach(storage_adapter);
    auto const status = contract_->DispatchQuery(query, request, response);
    contract_->Detach();

    return status;
  }

  Contract::Result InvokeInit(Identity const &                 owner,
                              fetch::chain::Transaction const &tx = fetch::chain::Transaction{})
  {
    StateSentinelAdapter storage_adapter{*storage_, *contract_name_, shards_};

    contract_->Attach(storage_adapter);
    auto const status =
        contract_->DispatchInitialise(fetch::chain::Address{owner}, tx, block_number_);
    contract_->Detach();

    return status;
  }

  fetch::BitVector const &shards() const
  {
    return shards_;
  }

  void shards(fetch::BitVector const &shards)
  {
    shards_ = shards;
  }

  /// @name User populated
  /// @{
  ContractPtr   contract_;
  AddressPtr    contract_address_;
  IdentifierPtr contract_name_;
  /// @}

  fetch::BitVector     shards_{FullShards(1)};
  Contract::BlockIndex block_number_{};
  CertificatePtr       certificate_;
  AddressPtr           owner_address_;
  MockStorageUnitPtr   storage_;
  TransactionPtr       tx_;
};
