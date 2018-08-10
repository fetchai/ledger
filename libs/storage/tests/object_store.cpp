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

/**
 * Test class used to verify that the object store can ser/deser objects correctly
 */
struct TestSerDeser
{
  int         first;
  uint64_t    second;
  std::string third;

  /**
   * Comparison operator
   *
   * @param: rhs Other test class
   *
   * @return: less than
   */
  bool operator<(TestSerDeser const &rhs) const { return third < rhs.third; }

  /**
   * Equality operator
   *
   * @param: rhs Other test class
   *
   * @return: is equal to
   */
  bool operator==(TestSerDeser const &rhs) const
  {
    return first == rhs.first && second == rhs.second && third == rhs.third;
  }
};

/**
 * Serializer for TestSerDeser
 *
 * @param: serializer The serializer
 * @param: b The class to serialize
 */
template <typename T>
inline void Serialize(T &serializer, TestSerDeser const &b)
{
  serializer << b.first;
  serializer << b.second;
  serializer << b.third;
}

/**
 * Deserializer for TestSerDeser
 *
 * @param: serializer The deserializer
 * @param: b The class to deserialize
 */
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

        for (std::size_t i = 0; i < iterations; ++i)
        {
          testStore.Set(ResourceAddress(std::to_string(i)), i);

          std::size_t result;

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
      std::vector<std::size_t> keyTests{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 99, 100, 1010, 9999};
      for (auto const &numberOfKeys : keyTests)
      {
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

        auto mmm = testStore.begin();
        while (mmm != testStore.end())
        {
          ++mmm;
        }

        auto nnn = testStore.begin();
        while (nnn != testStore.end())
        {
          ++nnn;
        }

        auto it = testStore.begin();

        while (it != testStore.end())
        {
          ++it;
        }

        it = testStore.begin();
        while (it != testStore.end())
        {
          objectsCopy.push_back(*it);
          ++it;
        }

        for (auto i : testStore)
        {
        }

        for (auto i : testStore)
        {
        }

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

        // Now, aim to split the store up and copy it across perfectly
        for (uint8_t keyBegin = 0; keyBegin < 16; ++keyBegin)
        {
          array[0] = (keyBegin << 4) & 0xFF;

          auto rid = ResourceID(array);

          testStore.Get(rid, dummy);

          auto it = testStore.GetSubtree(rid, uint64_t(4));

          while (it != testStore.end())
          {
            objectsCopy.push_back(*it);
            ++it;
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

  //SCENARIO("Testing object store basic functionality") // TODO: (`HUT`) : delete
  //{

    SECTION("Test subtree iterator over basic struct - split into 256 to emulate obj. sync")
    {
      std::vector<std::size_t> keyTests{23, 100, 1, 2, 3,  4, 5, 6, 7, 8, 9, 10, 11, 12,
                                        13, 14, 99, 9999, 0, 1, 9, 12, 14, 100, 1000};
      for (auto const &numberOfKeys : keyTests)
      {
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
        }

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
