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

#include "ledger/upow/synergetic_contract.hpp"
#include "math/bignumber.hpp"

#include "gtest/gtest.h"

namespace {

using fetch::math::BigUnsigned;

using namespace fetch::ledger;

class SynergeticContractTests : public ::testing::Test
{
public:

  void CreateContract(char const *text)
  {
    // create and parse the contract
    contract_ = std::make_shared<SynergeticContract>(text);
  }

  SynergeticContractPtr contract_;
};

TEST_F(SynergeticContractTests, SimpleTest)
{
  static char const *text = R"(
@problem
function dagState() : Int32
  return 0;
endfunction

@objective
function proofOfWork(problem : Int32, solution : Int32 ) : Int64
  return abs(toInt64(problem) - toInt64(solution));
endfunction

@clear
function applyWork(problem : Int32, solution : Int32)
endfunction

@work
function mineWork(problem : Int32, nonce : BigUInt) :  Int32
  return nonce.toInt32();
endfunction

@generator
function makeDAGnode(epoch : Int32, entropy : BigUInt)
endfunction
)";

  ASSERT_NO_THROW(CreateContract(text));
  EXPECT_EQ(contract_->problem_function(), "dagState");
  EXPECT_EQ(contract_->objective_function(), "proofOfWork");
  EXPECT_EQ(contract_->clear_function(), "applyWork");
  EXPECT_EQ(contract_->work_function(), "mineWork");
  EXPECT_EQ(contract_->generator_function(), "makeDAGnode");

  // create a simple nonce
  BigUnsigned nonce{42ull};

  // define the problem beforehand
  contract_->DefineProblem();

  // run a simple work cycle
  WorkScore score{0};
  EXPECT_EQ(contract_->Work(nonce, score), SynergeticContract::Status::SUCCESS);
  EXPECT_EQ(score, 42);

  contract_->Detach();
}

} // namespace