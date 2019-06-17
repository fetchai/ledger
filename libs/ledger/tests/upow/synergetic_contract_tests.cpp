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

#include "core/bitvector.hpp"
#include "ledger/upow/synergetic_contract.hpp"
#include "math/bignumber.hpp"

#include "mock_storage_unit.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace storage {

std::ostream &operator<<(std::ostream &s, ResourceAddress const &address)
{
  s << "0x" << address.id().ToHex() << " (" << address.address() << ")";
  return s;
}

}  // namespace storage
}  // namespace fetch

namespace {

using fetch::math::BigUnsigned;
using fetch::byte_array::ConstByteArray;
using fetch::storage::ResourceAddress;
using fetch::ledger::StorageUnitInterface;
using fetch::BitVector;
using ::testing::_;
using ::testing::StrictMock;

using StorageUnitInterfacePtr = std::unique_ptr<MockStorageUnit>;

using namespace fetch::ledger;

BitVector CreateShards()
{
  BitVector shards{1};
  shards.SetAllOne();

  return shards;
}

class SynergeticContractTests : public ::testing::Test
{
protected:
  static BitVector const SHARDS;

  void CreateContract(char const *text)
  {
    // create and parse the contract
    contract_ = std::make_shared<SynergeticContract>(text);
    storage_  = std::make_unique<StrictMock<MockStorageUnit>>();
  }

  void TearDown() override
  {
    contract_->Detach();
    contract_.reset();
    storage_.reset();
  }

  SynergeticContractPtr   contract_;
  StorageUnitInterfacePtr storage_;
};

BitVector const SynergeticContractTests::SHARDS{CreateShards()};

TEST_F(SynergeticContractTests, SimpleTest)
{
  static char const *text = R"(
@problem
function createProblem(data : Array<StructuredData>) : Int32
  var value = 0;
  for (i in 0:data.count() - 1)
    value += data[i].getInt32("value");
  endfor

  return value;
endfunction

@objective
function evaluateWork(problem : Int32, solution : Int32 ) : Int64
  return abs(toInt64(problem) - toInt64(solution));
endfunction

@work
function doWork(problem : Int32, nonce : BigUInt) :  Int32
  return nonce.toInt32();
endfunction

@clear
function applyWork(problem : Int32, solution : Int32)
  var result = State<Int32>("solution", 0);
  result.set(solution);
endfunction
)";

  ASSERT_NO_THROW(CreateContract(text));
  EXPECT_EQ(contract_->problem_function(), "createProblem");
  EXPECT_EQ(contract_->objective_function(), "evaluateWork");
  EXPECT_EQ(contract_->work_function(), "doWork");
  EXPECT_EQ(contract_->clear_function(), "applyWork");

  // create a simple nonce
  BigUnsigned nonce{42ull};

  // define the problem beforehand
  contract_->DefineProblem({
      R"({"value": 100})",
      R"({"value": 150})",
      R"({"value": 250})",
  });

  ASSERT_TRUE(contract_->HasProblem());
  auto const &problem = contract_->GetProblem();
  EXPECT_EQ(problem.primitive.i32, 500);

  // run a simple work cycle
  WorkScore score{0};
  EXPECT_EQ(contract_->Work(nonce, score), SynergeticContract::Status::SUCCESS);
  EXPECT_EQ(score, 458);

  // check the generated solution
  ASSERT_TRUE(contract_->HasSolution());
  auto const &solution = contract_->GetSolution();
  EXPECT_EQ(solution.primitive.i32, 42);

  // attach the
  contract_->Attach(*storage_);

  {
    uint64_t const block_number = 1024;

    // calculate the expected resources address
    ResourceAddress const resource_address{contract_->digest().ToHex() + "." +
                                           std::to_string(block_number) + ".state.solution"};

    EXPECT_CALL(*storage_, Lock(0));
    EXPECT_CALL(*storage_, Get(resource_address));
    EXPECT_CALL(*storage_, Set(resource_address, _));
    EXPECT_CALL(*storage_, Unlock(0));

    // complete the synergetic contract
    contract_->Complete(block_number, SHARDS);
  }

  contract_->Detach();

  ASSERT_FALSE(contract_->HasProblem());
  ASSERT_FALSE(contract_->HasSolution());
}

}  // namespace
