#include "serializer/referenced_byte_array.hpp"
#include <iostream>
#include <random>
#include "chain/transaction.hpp"
#include "random/lfg.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "serializer/stl_types.hpp"
using namespace fetch;
using namespace fetch::serializers;
using namespace fetch::byte_array;

using transaction = chain::Transaction;

fetch::random::LaggedFibonacciGenerator<> lfg;

template <typename T>
void MakeString(T &str, std::size_t N = 256) {
  byte_array::ByteArray entry;
  entry.Resize(N);

  for (std::size_t j = 0; j < N; ++j) {
    entry[j] = uint8_t(lfg() >> 19);
  }

  str = entry;
}

transaction NextTransaction() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(
      0, std::numeric_limits<uint32_t>::max());

  transaction trans;

  trans.PushGroup(group_type(dis(gen)));
  trans.PushGroup(group_type(dis(gen)));
  trans.PushGroup(group_type(dis(gen)));
  trans.PushGroup(group_type(dis(gen)));
  trans.PushGroup(group_type(dis(gen)));

  byte_array::ByteArray sig1, sig2, contract_name, arg1;
  MakeString(sig1);
  MakeString(sig2);
  MakeString(contract_name);
  MakeString(arg1, 4 * 256);

  trans.PushSignature(sig1);
  // trans.PushSignature(sig2);
  // trans.set_contract_name(contract_name);
  // trans.set_arguments(arg1);
  // trans.UpdateDigest();

  return trans;
}

transaction NextTransactionBreaks() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(
      0, std::numeric_limits<uint32_t>::max());

  transaction trans;

  trans.PushGroup(group_type(dis(gen)));
  trans.PushGroup(group_type(dis(gen)));
  trans.PushGroup(group_type(dis(gen)));
  trans.PushGroup(group_type(dis(gen)));
  trans.PushGroup(group_type(dis(gen)));

  byte_array::ByteArray sig1, sig2, contract_name, arg1;
  MakeString(sig1);
  MakeString(sig2);
  MakeString(contract_name);
  MakeString(arg1, 4 * 256);

  trans.PushSignature(sig1);
  trans.PushSignature(sig2);
  trans.set_contract_name(contract_name);
  trans.set_arguments(arg1);
  trans.UpdateDigest();

  return trans;
}

int main() {
  ByteArray str = "hello", str2 = "world";
  std::string nstr, nstr2;
  ByteArrayBuffer buffer;
  buffer << str << str2;
  buffer.Seek(0);
  buffer >> nstr >> nstr2;

  std::cout << nstr << std::endl << nstr2 << std::endl;

  {
    auto trans = NextTransaction();

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

  {
    auto trans = NextTransactionBreaks();

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
