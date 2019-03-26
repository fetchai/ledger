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

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/identity.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/contract.hpp"
#include "ledger/identifier.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/storage_unit/cached_storage_adapter.hpp"
#include "mock_storage_unit.hpp"

#include "ledger/fetch_msgpack.hpp"

#include <gtest/gtest.h>

class ContractTest : public ::testing::Test
{
protected:
  using Identity             = fetch::crypto::Identity;
  using Identifier           = fetch::ledger::Identifier;
  using MutableTransaction   = fetch::ledger::MutableTransaction;
  using VerifiedTransaction  = fetch::ledger::VerifiedTransaction;
  using ConstByteArray       = fetch::byte_array::ConstByteArray;
  using ByteArray            = fetch::byte_array::ByteArray;
  using Contract             = fetch::ledger::Contract;
  using ContractPtr          = std::shared_ptr<Contract>;
  using MockStorageUnitPtr   = std::unique_ptr<MockStorageUnit>;
  using Resources            = std::vector<ConstByteArray>;
  using CertificatePtr       = std::unique_ptr<fetch::crypto::ECDSASigner>;
  using StateAdapter         = fetch::ledger::StateAdapter;
  using StateSentinelAdapter = fetch::ledger::StateSentinelAdapter;
  using Query                = Contract::Query;
  using IdentifierPtr        = std::shared_ptr<Identifier>;
  using CachedStorageAdapter = fetch::ledger::CachedStorageAdapter;

  void SetUp() override
  {
    certificate_ = std::make_unique<fetch::crypto::ECDSASigner>();
    storage_     = std::make_unique<MockStorageUnit>();
  }

  void TearDown() override
  {
    contract_.reset();
    contract_name_.reset();
    storage_.reset();
    certificate_.reset();
  }

  class PayloadPacker
  {
  public:
    template <typename... Args>
    PayloadPacker(Args... args)
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

    void PackInternal(fetch::crypto::Identity const &identity)
    {
      auto const &id = identity.identifier();

      // pack as an address
      packer_.pack_ext(id.size(), static_cast<int8_t>(0x4D));
      packer_.pack_ext_body(id.char_pointer(), static_cast<uint32_t>(id.size()));
    }

    Buffer buffer_{};
    Packer packer_{&buffer_};
  };

  template <typename... Args>
  Contract::Status SendActionWithParams(ConstByteArray const &action, Resources const &resources,
                                        Args... args)
  {
    // pack all the data into a single payload
    PayloadPacker p{args...};

    return SendAction(action, resources, p.GetBuffer());
  }

  Contract::Status SendAction(ConstByteArray const &action, Resources const &resources,
                              ConstByteArray const &data = ConstByteArray{})
  {
    // create and sign the transaction
    MutableTransaction mtx{};
    mtx.set_contract_name(contract_name_->full_name() + "." + action);
    mtx.set_data(data);

    // add all the resources to the transaction
    for (auto const &resource : resources)
    {
      mtx.PushResource(resource);
    }

    mtx.Sign(certificate_->underlying_private_key());

    // create and finalise the transaction
    VerifiedTransaction tx = VerifiedTransaction::Create(std::move(mtx));

    // adapt the storage engine for this execution
    StateSentinelAdapter storage_adapter{*storage_, *contract_name_, tx.resources()};

    // dispatch the transaction to the contract
    contract_->Attach(storage_adapter);
    auto const status = contract_->DispatchTransaction(action, tx);
    contract_->Detach();

    return status;
  }

  Contract::Status SendAction(MutableTransaction const &mtx)
  {
    // create and finalise the transaction
    VerifiedTransaction tx = VerifiedTransaction::Create(mtx);

    Identifier full_contract_name{tx.contract_name()};

    // adapt the storage engine for this execution
    StateSentinelAdapter storage_adapter{*storage_, *contract_name_, tx.resources(),
                                         tx.resources()};

    // dispatch the transaction to the contract
    contract_->Attach(storage_adapter);
    auto const status = contract_->DispatchTransaction(full_contract_name.name(), tx);
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

  Contract::Status InvokeInit(Identity const &owner)
  {
    StateSentinelAdapter storage_adapter{*storage_, *contract_name_, {
      fetch::byte_array::ToBase64(owner.identifier())
    }};

    contract_->Attach(storage_adapter);
    auto const status = contract_->DispatchInitialise(owner);
    contract_->Detach();

    return status;
  }

  /// @name User populated
  /// @{
  ContractPtr   contract_;
  IdentifierPtr contract_name_;
  /// @}

  CertificatePtr     certificate_;
  MockStorageUnitPtr storage_;
};
