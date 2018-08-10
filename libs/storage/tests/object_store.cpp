#include "storage/extern_control.hpp"
#include "storage/object_store.hpp"
#include "core/random/lfg.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_serialization.hpp"
#include "testing/unittest.hpp"
#include <algorithm>
#include <iostream>

using namespace fetch::storage;
using namespace fetch::byte_array;

bool togglePrints = false;
int totalSize = 0;
std::string messageG = "";

// Reverse bits in byte
uint8_t Reverse(uint8_t c)
{
  c = uint8_t((c & 0xF0) >> 4) | uint8_t((c & 0x0F) << 4);
  c = uint8_t((c & 0xCC) >> 2) | uint8_t((c & 0x33) << 2);
  c = uint8_t((c & 0xAA) >> 1) | uint8_t((c & 0x55) << 1);
  return c;
}

struct TestSerDeser
{
  int         first;
  uint64_t    second;
  std::string third;

  bool operator<(TestSerDeser const &rhs) const { return third < rhs.third; }

  bool operator==(TestSerDeser const &rhs) const
  {
    return first == rhs.first && second == rhs.second && third == rhs.third;
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
  SCENARIO("Testing object store basic functionality")
  {
    SECTION("Setting and getting elements")
    {
      for (std::size_t iterations = 3; iterations < 10; ++iterations)
      {
        ObjectStore<std::size_t> testStore;
        testStore.New("testFile.db", "testIndex.db");
        //std::cout << "testing iterations: " << iterations << std::endl;

        for (std::size_t i = 0; i < iterations; ++i)
        {
          //std::cout << "" << std::endl;
  //        std::cout << "Setting " << i << std::endl;
          testStore.Set(ResourceAddress(std::to_string(i)), i);

          std::size_t result;

          //std::cout << "" << std::endl;
  //        std::cout << "Getting!" << std::endl;
          testStore.Get(ResourceAddress(std::to_string(i)), result);
  //        std::cout << "Getting2!" << std::endl;
          testStore.Get(ResourceAddress(std::to_string(i)), result);

          // Suppress most expects to avoid spamming terminal
          if(i != result || i == 0)
          {
            EXPECT(i == result);
          }
        }

        // Do a second run
        for (std::size_t i = 0; i < iterations; ++i)
        {
          std::size_t result;

          testStore.Get(ResourceAddress(std::to_string(i)), result);
          testStore.Get(ResourceAddress(std::to_string(i)), result);

          // Suppress most expects to avoid spamming terminal
          if(i != result || i == 0)
          {
            EXPECT(i == result);
          }
        }

        // Check against false positives
        for (std::size_t i = 1; i < iterations; ++i)
        {
          std::size_t result = 0;

          testStore.Get(ResourceAddress(std::to_string(i+iterations)), result);

          // Suppress most expects to avoid spamming terminal
          if(0 != result || i == 1)
          {
            EXPECT(0 == result);
          }
        }
      }

    };
  };

  SCENARIO("Testing object store with STL functionality")
  {
    SECTION("Test find over basic struct")
    {
      std::vector<std::size_t> keyTests{99, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100};

      for (auto const &numberOfKeys : keyTests)
      {
        //std::cout << "Testing keys: " << numberOfKeys << std::endl;
        using testType = TestSerDeser;
        ObjectStore<testType> testStore;
        testStore.New("testFile.db", "testIndex.db");

        std::vector<testType>                     objects;
        std::vector<testType>                     objectsCopy;
        fetch::random::LaggedFibonacciGenerator<> lfg;

        // Create vector of random numbers
        for (std::size_t i = 0; i < numberOfKeys; ++i)
        {
          uint64_t random = lfg();

          testType test;
          test.first  = int(-random);
          test.second = random;

          test.third = std::to_string(random);

          testStore.Set(ResourceAddress(test.third), test);
          objects.push_back(test);
        }

        std::sort(objects.begin(), objects.end());

        //togglePrints = true;
        ////testStore.Set(ResourceAddress("whooo"), testType());
        //testStore.Find(ResourceAddress("argh"));
        //togglePrints = false;

        // Test successfully finding and testing to find elements
        bool successfullyFound = true;
        int index = 0;
        for (auto const &i : objects)
        {
          auto it = testStore.Find(ResourceAddress(i.third));
          if (it == testStore.end())
          {
            successfullyFound = false;
            break;
          }

          index++;
        }

        EXPECT(successfullyFound == true && index == index);

        successfullyFound = false;

        for (std::size_t i = 0; i < 100; ++i)
        {
          auto it = testStore.Find(ResourceAddress(std::to_string(lfg())));

          if (it != testStore.end())
          {
            successfullyFound = true;
            break;
          }
        }

        EXPECT(successfullyFound == false);
      }
    };

    SECTION("Test find over basic struct, expect failures")
    {
      std::vector<std::size_t> keyTests{99, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100 };

      for (auto const &numberOfKeys : keyTests)
      {
        //std::cout << "Testing keys: " << numberOfKeys << std::endl;
        using testType = TestSerDeser;
        ObjectStore<testType> testStore;
        testStore.New("testFile.db", "testIndex.db");

        // Create vector of random numbers
        for (std::size_t i = 0; i < numberOfKeys; ++i)
        {
          testType test;
          test.first  = int(-i);
          test.second = i;

          test.third = std::to_string(i);

          testStore.Set(ResourceAddress(test.third), test);
        }

        bool successfullyFound = false;

        // Expect in the case of hash collisions, we shouldn't find these
        for (std::size_t i = numberOfKeys + 1; i < numberOfKeys * 2; ++i)
        {
          auto it = testStore.Find(ResourceAddress(std::to_string(i)));
          if (it != testStore.end())
          {
            successfullyFound = true;
            break;
          }
        }

        EXPECT(successfullyFound == false);
      }
    };

    SECTION("Test iterator over basic struct")
    {
      std::vector<std::size_t> keyTests{4,0,1,2,3,4,5,6,7,8,9,99, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 100, 11};
      for (auto const &numberOfKeys : keyTests)
      {
        //std::cout << "Testing keys: " << numberOfKeys << std::endl;
        using testType = TestSerDeser;
        ObjectStore<testType> testStore;
        testStore.New("testFile.db", "testIndex.db");

        std::vector<testType>                     objects;
        std::vector<testType>                     objectsCopy;
        fetch::random::LaggedFibonacciGenerator<> lfg;

        totalSize = int(numberOfKeys);

        // Create vector of random numbers
        for (std::size_t i = 0; i < numberOfKeys; ++i)
        {
          uint64_t random = lfg();

          testType test;
          test.first  = int(-random);
          test.second = random;

          test.third = std::to_string(random);

          testStore.Set(ResourceAddress(test.third), test);
          objects.push_back(test);

          messageG = "Build: ";
          messageG += std::to_string(i);

          togglePrints = true;
          testStore.PrintTree();
          togglePrints = false;
        }

        std::sort(objects.begin(), objects.end());

        //std::cout << "~~~~Should now iterate cleanly! " << numberOfKeys << std::endl;
        auto mmm = testStore.begin();
        while (mmm != testStore.end())
        {
          ++mmm;
        }
        //std::cout << "~~~~" << std::endl;

        auto nnn = testStore.begin();
        while (nnn != testStore.end())
        {
          ++nnn;
        }
        //std::cout << "====" << std::endl;

        int counter = 0;
        //std::cout << "Iterator as binary" << std::endl;
        auto it = testStore.begin();

        //if(togglePrints) std::cout << "Iterating over " << numberOfKeys << std::endl;

        while (it != testStore.end())
        {
          //std::cerr << "XXX" << ToBin(ResourceAddress((*it).third).id())  << "YYY"<< std::endl;
          togglePrints = true;
          //if(togglePrints)std::cout << "XXX" << ToBin(ResourceAddress((*it).third).id())  << "YYY"<< std::endl;
          togglePrints = false;
          ++it;
          counter++;
        }

        messageG = "Iterate2";

        //std::cout << "" << std::endl;

        //std::cout << counter << std::endl;
        counter = 0;

        it = testStore.begin();
        while (it != testStore.end())
        {
          objectsCopy.push_back(*it);
          ++it;
          counter++;
        }
        //std::cout << counter << std::endl;
        counter = 0;

        for (auto i : testStore)
        {
          counter++;
        }

        //std::cout << counter << std::endl;
        counter = 0;

        for (auto i : testStore)
        {
          counter++;
        }
        //std::cout << counter << std::endl;
        counter = 0;

        std::sort(objectsCopy.begin(), objectsCopy.end());

        EXPECT(objectsCopy.size() == objects.size());
        bool allMatch = std::equal(objectsCopy.begin(), objectsCopy.end(), objects.begin());
        EXPECT(allMatch == true);
      }
    };

    SECTION("Test subtree iterator over basic struct")
    {
      std::vector<std::size_t> keyTests{9, 1, 2, 3, 4, 5, 6, 7,  8,  9,   10,   11,   12,
                                        13, 14, 99, 9999, 0, 1, 9, 12, 14, 100, 1000, 10000};
      for (auto const &numberOfKeys : keyTests)
      {
        std::cout << "Testing keys: " << numberOfKeys << std::endl;
        using testType = TestSerDeser;
        ObjectStore<testType> testStore;
        testStore.New("testFile.db", "testIndex.db");

        std::vector<testType>                     objects;
        std::vector<testType>                     objectsCopy;
        fetch::random::LaggedFibonacciGenerator<> lfg;
        testType                                  dummy;

        ByteArray array;
        array.Resize(256 / 8);

        for (std::size_t i = 0; i < array.size(); ++i)
        {
          array[i] = 0;
        }

        // Create vector of random numbers
        for (std::size_t i = 0; i < numberOfKeys; ++i)
        {
          uint64_t random = lfg();

          testType test;
          test.first  = int(-random);
          test.second = random;

          test.third = std::to_string(random);

          testStore.Set(ResourceAddress(test.third), test);
          objects.push_back(test);
        }

        testStore.PrintTree();

        // Now, aim to split the store up and copy it across perfectly
        for (uint8_t keyBegin = 0; keyBegin < 16; ++keyBegin)
        {
          array[0] = (Reverse(keyBegin));

          auto rid = ResourceID(array);

          testStore.Get(rid, dummy);

          auto it = testStore.GetSubtree(rid, uint64_t(4));
          int counter = 0;

          //std::cout << "Key: " << std::endl;
          //std::cout << ToBin(rid.id()) << std::endl;

          while (it != testStore.end())
          {
            objectsCopy.push_back(*it);

            //std::cout << ToBin(ResourceAddress((*it).third).id()) << std::endl;
            ++it;
            counter++;
          }

          //std::cout << "Final count: " << counter << std::endl;
          //std::cout << "" << std::endl;
        }

        // expect iterator test to go well
        EXPECT(objectsCopy.size() == objects.size());

        std::sort(objects.begin(), objects.end());
        std::sort(objectsCopy.begin(), objectsCopy.end());

        bool allMatch = std::equal(objectsCopy.begin(), objectsCopy.end(), objects.begin());
        EXPECT(allMatch == true);
      }
    };

  //SCENARIO("Testing object store basic functionality") // TODO: (`HUT`) : delete
  //{

    SECTION("Test subtree iterator over basic struct - split into 256 to emulate obj. sync")
    {
      std::vector<std::size_t> keyTests{23, 100, 1, 2, 3,  4, 5, 6, 7, 8, 9, 10, 11, 12,
                                        13, 14, 99, 9999, 0, 1, 9, 12, 14, 100, 1000};
      for (auto const &numberOfKeys : keyTests)
      {
        //std::cout << "Testing keys: " << numberOfKeys << std::endl;
        using testType = TestSerDeser;
        ObjectStore<testType> testStore;
        testStore.New("testFile.db", "testIndex.db");

        std::vector<testType>                     objects;
        std::vector<testType>                     objectsCopy;
        fetch::random::LaggedFibonacciGenerator<> lfg;

        ByteArray array;
        array.Resize(256 / 8);

        for (std::size_t i = 0; i < array.size(); ++i)
        {
          array[i] = 0;
        }

        testType dummy;

        // Create vector of random numbers
        for (std::size_t i = 0; i < numberOfKeys; ++i)
        {
          uint64_t random = lfg();

          testType test;
          test.first  = int(-random);
          test.second = random;

          test.third = std::to_string(random);

          testStore.Set(ResourceAddress(test.third), test);
          objects.push_back(test);

          ////std::cout << ToBin(ResourceAddress(test.third).id()) << std::endl;
          ////std::cout << "" << std::endl;
          //std::cout << "." << std::endl;
          //std::cout << "" << std::endl;
        }

        //std::cout << "=====================================" << std::endl;

        // Now, aim to split the store up and copy it across perfectly
        for (uint8_t keyBegin = 0; ; ++keyBegin)
        {
          array[0] = (keyBegin);

          auto rid = ResourceID(array);

          testStore.Get(rid, dummy);

          auto it = testStore.GetSubtree(rid, uint64_t(8));

          while (it != testStore.end())
          {
            objectsCopy.push_back(*it);
            ++it;
          }

          if(keyBegin == 0xFF)
          {
            break;
          }
        }

        // expect iterator test to go well
        EXPECT(objectsCopy.size() == objects.size());

        std::sort(objects.begin(), objects.end());
        std::sort(objectsCopy.begin(), objectsCopy.end());

        bool allMatch = std::equal(objectsCopy.begin(), objectsCopy.end(), objects.begin());
        EXPECT(allMatch == true);
      }
    };
  };

  return 0;
}
