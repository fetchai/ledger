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

#include "core/byte_array/encoders.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "ledger/chain/helper_functions.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include <gtest/gtest.h>
#include <iostream>

using namespace fetch::ledger;
using namespace fetch::byte_array;

TEST(TransactionTypesTests, SerialisationOfTransactionsToConstTransaction)
{
  MutableTransaction trans;
  Transaction        tx;
  trans.PushResource("a");

  EXPECT_EQ(trans.resources().count("a"), 1);
  {
    VerifiedTransaction                 txTemp = VerifiedTransaction::Create(trans);
    fetch::serializers::ByteArrayBuffer arr;
    arr << txTemp;
    arr.seek(0);
    arr >> tx;
  }

  EXPECT_EQ(tx.resources().count("a"), 1);
}

TEST(TransactionTypesTests, RandomTransactionVerification)
{
  for (std::size_t i = 0; i < 10; ++i)
  {
    MutableTransaction        mutableTx   = fetch::ledger::RandomTransaction();
    const VerifiedTransaction transaction = VerifiedTransaction::Create(mutableTx);

#if 0
    std::cout << "\n= TX[" << std::setfill('0') << std::setw(5) << i
              << "] ==========================================" << std::endl;
    std::cout << "contract name:   " << transaction.contract_name() << std::endl;
    std::cout << "hash:            " << ToHex(transaction.summary().transaction_hash) << std::endl;
    std::cout << "data:            " << ToHex(transaction.data()) << std::endl;

    for (auto const &sig : transaction.signatures())
    {
      std::cout << "identity:        " << ToHex(sig.first.identifier()) << std::endl;
      std::cout << "identity params: " << sig.first.parameters() << std::endl;
      std::cout << "signature:       " << ToHex(sig.second.signature_data) << std::endl;
      std::cout << "signature type:  " << sig.second.type << std::endl;
    }
#endif

    EXPECT_TRUE(mutableTx.Verify());
  }
}