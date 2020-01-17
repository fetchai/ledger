//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/random/lfg.hpp"
#include "storage/single_object_store.hpp"
#include "storage/storage_exception.hpp"

#include "gtest/gtest.h"

using namespace fetch::storage;

class TestClass
{
public:
  uint64_t value1 = 0;
  uint8_t  value2 = 0;
  std::string name;

  bool operator==(TestClass const &rhs) const
  {
    return value1 == rhs.value1 && value2 == rhs.value2 && name == rhs.name;
  }
};

// Need to define a serializer for the single object store to use
namespace fetch {
namespace serializers {

template <typename D>
struct MapSerializer<TestClass, D>
{
public:
  using Type       = TestClass;
  using DriverType = D;

  static uint8_t const VALUE1 = 1;
  static uint8_t const VALUE2 = 2;
  static uint8_t const VALUE3 = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &ref)
  {
    auto map = map_constructor(3);
    map.Append(VALUE1, ref.value1);
    map.Append(VALUE2, ref.value2);
    map.Append(VALUE3, ref.name);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &ref)
  {
    map.ExpectKeyGetValue(VALUE1, ref.value1);
    map.ExpectKeyGetValue(VALUE2, ref.value2);
    map.ExpectKeyGetValue(VALUE3, ref.name);
  }
};

}  // namespace serializers
}  // namespace fetch

TEST(single_object_store, load_and_expect_throw)
{
  SingleObjectStore store;
  store.Load("single_obj_store_test1.db");
  store.Clear();

  // Expect a throw if attempt to get from an empty file
  TestClass testme;
  EXPECT_THROW(store.Get(testme), fetch::storage::StorageException);
}

TEST(single_object_store, load_and_expect_throw_wrong_data)
{
  SingleObjectStore store;
  store.Load("single_obj_store_test2.db");
  store.Clear();

  // Set a string
  store.Set(std::string{"a thing"});

  // Expect a throw if attempt to get some other type
  TestClass testme;
  EXPECT_THROW(store.Get(testme), std::runtime_error);
}

TEST(single_object_store, save_reload_expect_success)
{
  {
    SingleObjectStore store;
    store.Load("single_obj_store_test3.db");
    store.Clear();

    // Set a string
    store.Set(std::string{"a test case"});
  }

  // Re-load after destruction
  SingleObjectStore store2;
  store2.Load("single_obj_store_test3.db");

  std::string result;

  // Set a string
  store2.Get(result);

  EXPECT_EQ(result, std::string{"a test case"});
}

TEST(single_object_store, load_and_write_to_variable_sizes)
{
  constexpr uint64_t                        test_size = 1000;
  fetch::random::LaggedFibonacciGenerator<> lfg;

  SingleObjectStore clearme;
  clearme.Load("single_obj_store_test4.db");
  clearme.Clear();

  for (uint64_t i = 0; i < test_size; ++i)
  {
    TestClass ref;
    TestClass ref2;

    {
      SingleObjectStore store;
      store.Load("single_obj_store_test4.db");

      uint64_t  random = lfg();
      ref.value1 = static_cast<uint64_t>(random);
      ref.value2 = static_cast<uint8_t>(random);
      ref.name = std::to_string(i); // Variable serialized size due to this

      store.Set(ref);
    }

    SingleObjectStore store;
    store.Load("single_obj_store_test4.db");
    store.Get(ref2);

    EXPECT_EQ(ref, ref2);
  }
}
