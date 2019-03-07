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
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/contract.hpp"
#include "ledger/identifier.hpp"
#include "ledger/state_sentinel.hpp"
#include "mock_storage_unit.hpp"

#include <gtest/gtest.h>

class ContractTest : public ::testing::Test
{
protected:
  using Identifier           = fetch::ledger::Identifier;
  using MutableTransaction   = fetch::ledger::MutableTransaction;
  using VerifiedTransaction  = fetch::ledger::VerifiedTransaction;
  using ConstByteArray       = fetch::byte_array::ConstByteArray;
  using Contract             = fetch::ledger::Contract;
  using ContractPtr          = std::shared_ptr<Contract>;
  using MockStorageUnitPtr   = std::unique_ptr<MockStorageUnit>;
  using Resources            = std::vector<ConstByteArray>;
  using CertificatePtr       = std::unique_ptr<fetch::crypto::ECDSASigner>;
  using StateSentinelAdapter = fetch::ledger::StateSentinelAdapter;
  using StateAdapter         = fetch::ledger::StateAdapter;
  using Query                = Contract::Query;
  using IdentifierPtr        = std::shared_ptr<Identifier>;

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
    StateSentinelAdapter storage_adapter{*storage_, *contract_name_, tx.resources()};

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

  /// @name User populated
  /// @{
  ContractPtr   contract_;
  IdentifierPtr contract_name_;
  /// @}

  CertificatePtr     certificate_;
  MockStorageUnitPtr storage_;
};