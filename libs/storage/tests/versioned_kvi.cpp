#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/random/lfg.hpp"
#include "storage/key.hpp"
#include "storage/key_value_index.hpp"
#include "testing/unittest.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
using namespace fetch;
using namespace fetch::storage;
// typedef KeyValueIndex< KeyValuePair< >, CachedRandomAccessStack<
// KeyValuePair< > > > kvi_type;
using kvi_type = KeyValueIndex<>;

kvi_type key_index;

struct TestData
{
  byte_array::ByteArray key;
  uint64_t              value;
};

std::map<byte_array::ConstByteArray, uint64_t> reference;
fetch::random::LaggedFibonacciGenerator<>      lfg;

int main()
{
  std::vector<TestData> bookmarks;

  std::vector<TestData> values;
  for (std::size_t i = 0; i < 5; ++i)
  {
    byte_array::ByteArray key;
    key.Resize(256 / 8);
    for (std::size_t j = 0; j < key.size(); ++j)
    {
      key[j] = uint8_t(lfg() >> 9);
    }

    if (reference.find(key) != reference.end()) continue;

    reference[key] = lfg();
    values.push_back({key, reference[key]});
  }

  // Insterting data
  key_index.New("test1.db", "diff.db");
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val = values[i];
    key_index.Set(val.key, val.value, val.key);
    if ((i % 2) == 0)
    {
      byte_array::ByteArray h1 = key_index.Hash();
      TestData              book;
      book.key   = h1;
      book.value = key_index.Commit();
      bookmarks.push_back(book);
      if (h1 != key_index.Hash())
      {
        std::cerr << "Expected hash to be the same before and after commit" << std::endl;
        exit(-1);
      }
    }
  }

  // Checking values
  bool ok = true;
  for (std::size_t i = 0; i < values.size(); ++i)
  {
    auto const &val  = values[i];
    uint64_t    data = key_index.Get(val.key);
    if (data != val.value)
    {
      std::cout << "Error for " << i << std::endl;
      std::cout << "Expected: " << val.value << std::endl;
      std::cout << "Got: " << data << std::endl;
      return -1;
    }
    ok &= (data == val.value);
  }

  // Reverting
  std::reverse(bookmarks.begin(), bookmarks.end());
  for (auto &b : bookmarks)
  {
    key_index.Revert(b.value);
    std::cout << "Reverting: " << b.value << " " << byte_array::ToBase64(b.key) << " "
              << byte_array::ToBase64(key_index.Hash()) << std::endl;
    if (b.key != key_index.Hash())
    {
      std::cout << "Expected " << std::endl;
      exit(-1);
    }
  }

  return ok ? 0 : -1;
}
