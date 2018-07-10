#include <iostream>
#include "core/byte_array/encoders.hpp"
#include "ledger/chain/basic_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/mutable_transaction.hpp"
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
      MutableTransaction      trans;
      Transaction             tx;
      fetch::group_type val = fetch::byte_array::BasicByteArray("a");

      trans.PushGroup(val);

      EXPECT(trans.groups()[0] == fetch::byte_array::BasicByteArray("a"));

      {
        Transaction txTemp = MakeTransaction(std::move(trans));
        fetch::serializers::ByteArrayBuffer arr;
        arr << txTemp;
        arr.Seek(0);
        arr >> tx;
      }

      EXPECT(tx.groups()[0] == fetch::byte_array::BasicByteArray("a"));
    };

  };

  return 0;
}
