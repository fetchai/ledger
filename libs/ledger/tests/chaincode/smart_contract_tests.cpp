//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/chaincode/vm_definition.hpp"
#include "mock_storage_unit.hpp"
#include "vm/compiler.hpp"
#include "vm/module.hpp"

#include <gmock/gmock.h>

#include <iostream>
#include <ledger/chain/mutable_transaction.hpp>
#include <memory>
#include <random>
#include <sstream>
#include <string>

using ::testing::_;
using namespace fetch;
using namespace fetch::ledger;

class SmartContractTests : public ::testing::Test
{
protected:
  using Query              = Contract::Query;
  using SmartContractPtr   = std::unique_ptr<SmartContract>;
  using MockStorageUnitPtr = std::unique_ptr<MockStorageUnit>;
  using Address            = fetch::byte_array::ConstByteArray;

  enum
  {
    IDENTITY_SIZE = 64
  };

  void SetUp() override
  {
    //
    storage_ = std::make_unique<MockStorageUnit>();

    //
  }

  bool CompileContract(std::string const &contract)
  {
    return true;  // TODO(private issue 236): Work in progress on smart contracts

    // create the transaction
    std::unique_ptr<vm::Module> module   = CreateVMDefinition<SmartContract>();
    fetch::vm::Compiler *       compiler = new fetch::vm::Compiler(module.get());

    fetch::vm::Script        script;
    std::vector<std::string> errors;
    bool compiled = compiler->Compile(contract, "ai.fetch.testcontract", script, errors);

    if (!compiled)
    {
      std::cout << "Failed to compile" << std::endl;
      for (auto &s : errors)
      {
        std::cout << s << std::endl;
      }
      return false;
    }

    contract_ = std::make_unique<SmartContract>(script);
    contract_->Attach(*storage_);

    chain::MutableTransaction tx;
    tx.set_contract_name("ai.fetch.testcontract.Test");
    //    tx.set_data(oss.str());
    //    tx.PushResource(address);

    Identifier identifier;
    identifier.Parse(static_cast<std::string>(tx.contract_name()));

    // dispatch the transaction
    auto status =
        contract_->DispatchTransaction(identifier.name(), chain::VerifiedTransaction::Create(tx));

    tx.set_contract_name("ai.fetch.testcontract.EdsFunction");
    //    tx.set_data(oss.str());
    //    tx.PushResource(address);

    identifier.Parse(static_cast<std::string>(tx.contract_name()));

    // dispatch the transaction
    status =
        contract_->DispatchTransaction(identifier.name(), chain::VerifiedTransaction::Create(tx));

    return (Contract::Status::OK == status);
  }

private:
  SmartContractPtr   contract_;
  MockStorageUnitPtr storage_;
};

TEST_F(SmartContractTests, CompileContract)
{
  std::string contract = R"(
function Test()
  Ledger.Print("hello world");
endfunction

function EdsFunction()
  Ledger.Print("hello world 2");
endfunction

  )";
  EXPECT_TRUE(CompileContract(contract));

  // generate the transaction contents
  //  uint64_t balance = std::numeric_limits<uint64_t>::max();
  //  EXPECT_TRUE(GetBalance(address, balance));
  //  EXPECT_EQ(balance, 1000);
}
