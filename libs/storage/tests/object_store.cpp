#include "testing/unittest.hpp"
#include "storage/object_store.hpp"
#include "core/random/lfg.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include <iostream>
#include <algorithm>

using namespace fetch::storage;
using namespace fetch::byte_array;

struct TestSerDeser
{
  int         first;
  uint64_t    second;
  std::string third;

  bool operator<(TestSerDeser const &rhs)
  {
    return third < rhs.third;
  }
};

template <typename T>
inline void Serialize(T &serializer, TestSerDeser const &b)
{
  serializer << b.first;
  serializer << b.second;
  serializer << b.third;
}

template <typename T>
inline void Deserialize(T &serializer, TestSerDeser &b)
{
  serializer >> b.first;
  serializer >> b.second;
  std::string ret;
  serializer >> ret;
  b.third = ret;
}

int main(int argc, char const **argv)
{

  SCENARIO("Testing object store with primitives")
  {
  //  SECTION("Test 1")
  //  {
  //    // Run this twice to check we correctly clear state between runs
  //    for (std::size_t i = 0; i < 2; ++i)
  //    {
  //      ObjectStore<int> testStore;
  //      testStore.New("testFile.db", "testIndex.db");

  //      int x = 1283112;
  //      int y = 0;

  //      // Expect this file to not contain 
  //      EXPECT(testStore.Get(ResourceID("testSet"), y) == false);
  //      EXPECT(y == 0);

  //      testStore.Set(ResourceID("testSet"), x);

  //      testStore.Get(ResourceID("testSet"), y);
  //      //EXPECT(testStore.Get(ResourceID("testSet"), y) == true);
  //      EXPECT(y == x);
  //    }
  //  };

    //SECTION("Test iterator over keys")
    //{
    //  ObjectStore<int> testStore;
    //  testStore.New("testFile.db", "testIndex.db");

    //  std::vector<std::string> keys;
    //  fetch::random::LaggedFibonacciGenerator<> lfg;

    //  // Create vector of random numbers
    //  //for (std::size_t i = 0; i < std::numeric_limits<std::size_t>::max()/4; ++i)
    //  for (std::size_t i = 0; i < 3; ++i)
    //  {
    //    int random = int(lfg());

    //    std::cout << "===================================" << std::endl;
    //    std::string key = std::to_string(random);
    //    key += "_key";
    //    testStore.Set(ResourceID(key), random);
    //    std::cout << "+++++++++++++++++++++++++++++++++++" << std::endl;
    //    keys.push_back(key);
    //  }

    //  std::cout << "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII" << std::endl;

    //  std::sort(keys.begin(), keys.end());

    //  for(auto const &i : keys)
    //  {
    //    int dummy;
    //    std::cout << "############ normal get" <<  std::endl;
    //    testStore.Get(ResourceID(i), dummy);

    //    std::cout << i << ":" << dummy << std::endl;

    //    std::cout << "############ find" << std::endl;
    //    auto it = testStore.Find(ResourceID(i));
    //    //auto obj = it.Get();

    //    std::cout << "############ dereference" << std::endl;
    //    auto obj = *it;

    //    std::cout << "name 2 " << obj << std::endl;

    //    //EXPECT(ToHex(i.contract_name()) == ToHex((*it).contract_name()));
    //  }

    //  std::cout << "finished" << std::endl;

    //  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
    //};

  };

  SCENARIO("Testing object store with classes")
  {

    //SECTION("Test iterator over basic struct")
    //{
    //  using testType = TestSerDeser;
    //  ObjectStore<testType> testStore;
    //  testStore.New("testFile.db", "testIndex.db");

    //  std::vector<testType> objects;
    //  fetch::random::LaggedFibonacciGenerator<> lfg;

    //  // Create vector of random numbers
    //  //for (std::size_t i = 0; i < std::numeric_limits<std::size_t>::max()/4; ++i)
    //  for (std::size_t i = 0; i < 3; ++i)
    //  {
    //    uint64_t random = lfg();

    //    testType test;
    //    test.first = int(-random);
    //    test.second = random;
    //    test.third = std::to_string(random);

    //    std::cout << "===================================" << std::endl;
    //    testStore.Set(ResourceID(test.third), test);
    //    std::cout << "+++++++++++++++++++++++++++++++++++" << std::endl;
    //    objects.push_back(test);

    //    std::cout << ">name 1   " << test.first << std::endl;
    //    std::cout << ">name 2   " << test.second << std::endl;
    //    std::cout << ">name 3   " << test.third << std::endl;
    //  }

    //  std::cout << "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII" << std::endl;

    //  //std::sort(objects.begin(), objects.end());

    //  for(auto const &i : objects)
    //  {
    //    testType dummy;
    //    std::cout << "############ normal get" <<  std::endl;
    //    testStore.Get(ResourceID(i.third), dummy);

    //    std::cout << "############ find" << std::endl;
    //    auto it = testStore.Find(ResourceID(i.third));
    //    //auto obj = it.Get();

    //    std::cout << "############ dereference" << std::endl;
    //    auto obj = *it;

    //    std::cout << "name ref " << i.third << std::endl;
    //    std::cout << "name 1   " << obj.first << std::endl;
    //    std::cout << "name 2   " << obj.second << std::endl;
    //    std::cout << "name 3   " << obj.third << std::endl;
    //  }

    //  std::cout << "finished" << std::endl;

    //  //for(auto &i : testStore)
    //  //{
    //  //}
    //  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
    //};

    SECTION("Test iterator over basic struct")
    {
      using testType = TestSerDeser;
      ObjectStore<testType> testStore;
      testStore.New("testFile.db", "testIndex.db");

      std::vector<testType> objects;
      fetch::random::LaggedFibonacciGenerator<> lfg;

      // Create vector of random numbers
      //for (std::size_t i = 0; i < std::numeric_limits<std::size_t>::max()/4; ++i)
      for (std::size_t i = 0; i < 600; ++i)
      {
        uint64_t random = lfg();

        testType test;
        test.first = int(-random);
        test.second = random;
        test.third = std::to_string(random);

        testStore.Set(ResourceID(test.third), test);
        objects.push_back(test);
      }

      std::sort(objects.begin(), objects.end());

      std::cout << "Ordered keys:" << std::endl;
      for(auto const &i : objects)
      {
        std::cout << i.third << std::endl;
      }

      std::cout  << std::endl;
      std::cout << "Iterated:" << std::endl;

      auto it = testStore.begin();
      int count = 0;
      while(it != testStore.end() && count < 10)
      {
        std::cout << (*it).third << std::endl;
        it++;
        //count++;
      }
    };

    //SECTION("Test iterator over keys")
    //{
    //  // TODO: (`HUT`) : we should have TX tests to ensure their hash is same when converting
    //  // between types
    //  using txMutable = fetch::chain::MutableTransaction;
    //  using tx        = fetch::chain::VerifiedTransaction;
    //  ObjectStore<tx> testStore;
    //  testStore.New("testFile.db", "testIndex.db");

    //  std::vector<tx> transactions;
    //  fetch::random::LaggedFibonacciGenerator<> lfg;

    //  // Create vector of random numbers
    //  //for (std::size_t i = 0; i < std::numeric_limits<std::size_t>::max()/4; ++i)
    //  for (std::size_t i = 0; i < 3; ++i)
    //  {
    //    uint64_t random = lfg();
    //    txMutable transactionMutable;
    //    transactionMutable.set_contract_name(std::to_string(random));

    //    tx transaction = tx::Create(transactionMutable);

    //    std::cout << "===================================" << std::endl;
    //    testStore.Set(ResourceID(transaction.digest()), transaction);
    //    std::cout << "+++++++++++++++++++++++++++++++++++" << std::endl;
    //    transactions.push_back(transaction);
    //  }

    //  std::cout << "IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII" << std::endl;

    //  std::sort(transactions.begin(), transactions.end());

    //  for(auto const &i : transactions)
    //  {
    //    tx dummy;
    //    std::cout << "############ normal get" <<  std::endl;
    //    testStore.Get(ResourceID(i.digest()), dummy);

    //    std::cout << "############ find" << std::endl;
    //    auto it = testStore.Find(ResourceID(i.digest()));
    //    //auto obj = it.Get();

    //    std::cout << "############ dereference" << std::endl;
    //    auto obj = *it;

    //    std::cout << "name 1 " << i.contract_name() << std::endl;
    //    std::cout << "name 2 " << (*it).contract_name() << std::endl;

    //    EXPECT(ToHex(i.contract_name()) == ToHex((*it).contract_name()));
    //  }

    //  std::cout << "finished" << std::endl;

    //  //for(auto &i : testStore)
    //  //{
    //  //}
    //  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
    //};

  };

  return 0;
}
