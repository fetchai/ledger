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
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include <iostream>

#include "ledger/chain/helper_functions.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include "testing/unittest.hpp"

using namespace fetch::chain;
using namespace fetch::byte_array;

int main(int argc, char const **argv)
{

  //  SCENARIO("testing creation of transactions")
  //  {
  //
  //    SECTION("Modifiable transaction")
  //    {
  //      Transaction trans;
  //
  //      fetch::group_type val = 5;
  //
  //      trans.PushGroup(val++);
  //      trans.PushGroup(val++);
  //      trans.PushGroup(val++);
  //      trans.UpdateDigest();
  //
  //      for(auto const &i: trans.groups())
  //      {
  //        std::cout << "Group: " << i << std::endl;
  //      }
  //
  //      EXPECT(trans.groups()[0] == 5);
  //    };
  //
  //    SECTION("Const transaction")
  //    {
  //      Transaction trans;
  //
  //      fetch::group_type val = 6;
  //
  //      trans.PushGroup(val++);
  //
  //      ConstTransaction constTran = MakeConstTrans(trans);
  //
  //      EXPECT(constTran.groups()[0] == 6);
  //    };
  //
  //  };

  SCENARIO("testing ser/deser transactions")
  {

    SECTION("Ser/deser transactions into ConstTransaction")
    {
      MutableTransaction trans;
      Transaction        tx;

      trans.PushResource("a");

      EXPECT(trans.resources().count("a") == 1);

      {
        VerifiedTransaction                 txTemp = VerifiedTransaction::Create(trans);
        fetch::serializers::ByteArrayBuffer arr;
        arr << txTemp;
        arr.Seek(0);
        arr >> tx;
      }

      EXPECT(tx.resources().count("a") == 1);
    };

    SECTION("Random transaction generation")
    {
      for (std::size_t i = 0; i < 10; ++i)
      {
        MutableTransaction mutableTx = fetch::chain::RandomTransaction();

        const VerifiedTransaction transaction = VerifiedTransaction::Create(mutableTx);

        std::cout << "\n===========================================" << std::endl;
        std::cout << ToHex(transaction.summary().transaction_hash) << std::endl;
        std::cout << ToHex(transaction.data()) << std::endl;
        for (auto const &sig : transaction.signatures())
        {
          std::cout << ToHex(sig.first.identifier()) << std::endl;
          std::cout << ToHex(sig.first.parameters()) << std::endl;
          std::cout << ToHex(sig.second.signature_data) << std::endl;
          std::cout << ToHex(sig.second.type) << std::endl;
        }
        std::cout << transaction.contract_name() << std::endl;
      }
    };
  };

  return 0;
}
