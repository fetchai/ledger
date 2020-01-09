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
#include "chain/transaction.hpp"
#include "chain/transaction_builder.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/contract_context_attacher.hpp"
#include "ledger/chaincode/deed.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/state_sentinel_adapter.hpp"
#include "ledger/storage_unit/fake_storage_unit.hpp"
#include "ledger/transaction_validator.hpp"

#include "gtest/gtest.h"

#include <initializer_list>

namespace fetch {
namespace ledger {

std::ostream &operator<<(std::ostream &stream, ContractExecutionStatus status)
{
  stream << ToString(status);
  return stream;
}

}  // namespace ledger
}  // namespace fetch

namespace {

using fetch::BitVector;
using fetch::chain::Address;
using fetch::chain::TransactionBuilder;
using fetch::crypto::ECDSASigner;
using fetch::ledger::ContractContext;
using fetch::ledger::ContractContextAttacher;
using fetch::ledger::ContractExecutionStatus;
using fetch::ledger::FakeStorageUnit;
using fetch::ledger::StateSentinelAdapter;
using fetch::ledger::TokenContract;
using fetch::ledger::TransactionValidator;
using fetch::ledger::Deed;

class DeedBuilder
{
public:
  using AddressList = std::initializer_list<Address>;

  DeedBuilder &AddSignee(Address const &address, Deed::Weight weight)
  {
    signees_[address] = weight;
    return *this;
  }

  DeedBuilder &AddOperation(Deed::Operation const &operation, Deed::Threshold thresold)
  {
    thresholds_[operation] = thresold;
    return *this;
  }

  Deed Build()
  {
    return {signees_, thresholds_};
  }

private:
  Deed::Signees            signees_;
  Deed::OperationTresholds thresholds_;
};

class TransactionValidatorTests : public ::testing::Test
{
protected:
  using DeedPtr = std::shared_ptr<Deed>;

  static void SetUpTestCase()
  {
    fetch::chain::InitialiseTestConstants();
  }

  void AddFunds(uint64_t amount);
  void SetDeed(Deed const &deed);

  ECDSASigner          signer_;
  Address              signer_address_{signer_.identity()};
  TokenContract        token_contract_;
  FakeStorageUnit      storage_;
  TransactionValidator validator_{storage_, token_contract_};
};

void TransactionValidatorTests::AddFunds(uint64_t amount)
{
  // create shard mask
  BitVector shards{1};
  shards.SetAllOne();

  // create storage infrastructure
  StateSentinelAdapter    storage_adapter{storage_, "fetch.token", shards};
  ContractContext         ctx{nullptr, Address{}, nullptr, &storage_adapter, 0};
  ContractContextAttacher attacher{token_contract_, ctx};

  // add the tokens to the account
  token_contract_.AddTokens(signer_address_, amount);
}

void TransactionValidatorTests::SetDeed(Deed const &deed)
{
  // create shard mask
  BitVector shards{1};
  shards.SetAllOne();

  // create storage infrastructure
  StateSentinelAdapter    storage_adapter{storage_, "fetch.token", shards};
  ContractContext         ctx{nullptr, Address{}, nullptr, &storage_adapter, 0};
  ContractContextAttacher attacher{token_contract_, ctx};

  // add the tokens to the account
  token_contract_.SetDeed(signer_address_, std::make_shared<Deed>(deed));
}

// TODO(HUT): fix these.
TEST_F(TransactionValidatorTests, CheckWealthWhileValid)
{
  uint64_t funds_for_test = 10000;
  AddFunds(funds_for_test);

  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("fetch.token", BitVector{})
                .Action("foo-bar-baz")
                .ValidUntil(100)
                .Signer(signer_.identity())
                .ChargeRate(1)
                .ChargeLimit(funds_for_test)
                .Seal()
                .Sign(signer_)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::SUCCESS, validator_(*tx, 50));
  EXPECT_EQ(ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK, validator_(*tx, 100));
  EXPECT_EQ(ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK, validator_(*tx, 101));
}

TEST_F(TransactionValidatorTests, CheckWealthOnValidityBoundary)
{
  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("fetch.token", BitVector{})
                .Action("wealth")
                .ValidUntil(100)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK, validator_(*tx, 100));
}

TEST_F(TransactionValidatorTests, CheckWealthOutsideOfValidityPeriod)
{
  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("fetch.token", BitVector{})
                .Action("wealth")
                .ValidUntil(100)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK, validator_(*tx, 1000));
}

TEST_F(TransactionValidatorTests, CheckDefaultCaseOnValidityBoundary)
{
  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK, validator_(*tx, 100));
}

TEST_F(TransactionValidatorTests, CheckNoChargeRate)
{
  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::TX_NOT_ENOUGH_CHARGE, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckNoChargeLimit)
{
  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .ChargeRate(1)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::TX_NOT_ENOUGH_CHARGE, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckDefaultCase)
{
  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .ChargeRate(1)
                .ChargeLimit(1)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();

  AddFunds(1);

  EXPECT_EQ(ContractExecutionStatus::SUCCESS, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckNoEnoughCharge)
{
  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .ChargeRate(100)
                .ChargeLimit(1)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();

  AddFunds(1);

  EXPECT_EQ(ContractExecutionStatus::INSUFFICIENT_AVAILABLE_FUNDS, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckPermissionDeniedIncorrectSignature)
{
  ECDSASigner other1;

  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .ChargeRate(100)
                .ChargeLimit(1)
                .Signer(other1.identity())
                .Seal()
                .Sign(other1)
                .Build();

  AddFunds(1);

  EXPECT_EQ(ContractExecutionStatus::TX_PERMISSION_DENIED, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckPermissionDeniedTooManySignatures)
{
  ECDSASigner other1;
  ECDSASigner other2;

  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .ChargeRate(100)
                .ChargeLimit(1)
                .Signer(other1.identity())
                .Signer(other2.identity())
                .Seal()
                .Sign(other1)
                .Sign(other2)
                .Build();

  AddFunds(1);

  EXPECT_EQ(ContractExecutionStatus::TX_PERMISSION_DENIED, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckOkayWithDeedWithExecute)
{
  ECDSASigner other1;
  ECDSASigner other2;

  Address address1{other1.identity()};
  Address address2{other2.identity()};

  // build the deed
  auto deed = DeedBuilder{}
                  .AddSignee(address1, 1)
                  .AddSignee(address2, 1)
                  .AddOperation(Deed::TRANSFER, 2)
                  .AddOperation(Deed::EXECUTE, 2)
                  .Build();
  SetDeed(deed);
  AddFunds(1);

  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .ChargeRate(1)
                .ChargeLimit(1)
                .Signer(other1.identity())
                .Signer(other2.identity())
                .Seal()
                .Sign(other1)
                .Sign(other2)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::SUCCESS, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckOkayWithDeedTransferOnly)
{
  ECDSASigner other1;
  ECDSASigner other2;

  Address address1{other1.identity()};
  Address address2{other2.identity()};

  // build the deed
  auto deed = DeedBuilder{}
                  .AddSignee(address1, 1)
                  .AddSignee(address2, 1)
                  .AddOperation(Deed::TRANSFER, 2)
                  .Build();
  SetDeed(deed);
  AddFunds(12);

  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .ValidUntil(100)
                .ChargeRate(1)
                .ChargeLimit(2)
                .Transfer(address1, 5)
                .Transfer(address2, 5)
                .Signer(other1.identity())
                .Signer(other2.identity())
                .Seal()
                .Sign(other1)
                .Sign(other2)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::SUCCESS, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckNotEnoughChargeTransfers)
{
  ECDSASigner other1;
  ECDSASigner other2;

  Address address1{other1.identity()};
  Address address2{other2.identity()};

  AddFunds(12);

  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .ValidUntil(100)
                .ChargeRate(1)
                .ChargeLimit(2)
                .Transfer(address1, 5)
                .Transfer(address2, 5)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::SUCCESS, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckPermissionDeniedWithDeedNoTransferPermission)
{
  ECDSASigner other1;
  ECDSASigner other2;

  Address address1{other1.identity()};
  Address address2{other2.identity()};

  // build the deed
  auto deed = DeedBuilder{}.AddSignee(address1, 1).AddSignee(address2, 1).Build();
  SetDeed(deed);
  AddFunds(12);

  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .ValidUntil(100)
                .ChargeRate(1)
                .ChargeLimit(2)
                .Transfer(address1, 5)
                .Transfer(address2, 5)
                .Signer(other1.identity())
                .Signer(other2.identity())
                .Seal()
                .Sign(other1)
                .Sign(other2)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::TX_PERMISSION_DENIED, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckPermissionDeniedWithDeedNoExecutePermission)
{
  ECDSASigner other1;
  ECDSASigner other2;

  Address address1{other1.identity()};
  Address address2{other2.identity()};

  // build the deed
  auto deed = DeedBuilder{}
                  .AddSignee(address1, 1)
                  .AddSignee(address2, 1)
                  .AddOperation(Deed::TRANSFER, 2)
                  .Build();
  SetDeed(deed);
  AddFunds(1);

  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetSmartContract(address2, BitVector{})  // reuse addresses for contract id
                .Action("do.work")
                .ValidUntil(100)
                .ChargeRate(1)
                .ChargeLimit(1)
                .Signer(other1.identity())
                .Signer(other2.identity())
                .Seal()
                .Sign(other1)
                .Sign(other2)
                .Build();

  EXPECT_EQ(ContractExecutionStatus::TX_PERMISSION_DENIED, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckBorderlineChargeLimit)
{
  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .ChargeRate(1)
                .ChargeLimit(10000000000)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();
  AddFunds(20000000000);

  EXPECT_EQ(ContractExecutionStatus::SUCCESS, validator_(*tx, 50));
}

TEST_F(TransactionValidatorTests, CheckExcessiveChargeLimit)
{
  auto tx = TransactionBuilder{}
                .From(signer_address_)
                .TargetChainCode("some.kind.of.chain.code", BitVector{})
                .Action("do.work")
                .ValidUntil(100)
                .ChargeRate(1)
                .ChargeLimit(10000000001)
                .Signer(signer_.identity())
                .Seal()
                .Sign(signer_)
                .Build();
  AddFunds(20000000000);

  EXPECT_EQ(ContractExecutionStatus::TX_CHARGE_LIMIT_TOO_HIGH, validator_(*tx, 50));
}

}  // namespace
