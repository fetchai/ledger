#include "serializer/referenced_byte_array.hpp"
#include <iostream>
#include <random>
#include "chain/transaction.hpp"
#include "random/lfg.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/stl_types.hpp"
#include "common/helper_functions.hpp"
using namespace fetch;
using namespace fetch::serializers;
using namespace fetch::byte_array;
using namespace fetch::common;

using transaction = chain::Transaction;

int main() {
  ByteArray str = "hello", str2 = "world";
  std::string nstr, nstr2;
  ByteArrayBuffer buffer;
  buffer << str << str2;
  buffer.Seek(0);
  buffer >> nstr >> nstr2;

  std::cout << nstr << std::endl << nstr2 << std::endl;

  {
    transaction trans = NextTransaction<transaction>();

    std::cout << "groups are: " << trans.summary().groups.size() << std::endl;
    ByteArrayBuffer testBuffer;
    testBuffer << trans;
    testBuffer.Seek(0);

    fetch::chain::Transaction result;
    testBuffer >> result;
    std::cout << "groups are: " << result.summary().groups.size() << std::endl;

    for (auto &i : result.summary().groups) {
      std::cout << i << std::endl;
    }
  }

  return 0;
}
