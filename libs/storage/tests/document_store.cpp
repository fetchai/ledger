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
#include "storage/document.hpp"
#include "storage/revertible_document_store.hpp"
#include <iostream>
using namespace fetch;

using namespace fetch::storage;
class TestStore : public RevertibleDocumentStore
{
};

TestStore store;

uint64_t book = 1;

void Set(std::string const &key, std::string const &val)
{
  std::cout << "Setting: " << key << " -> " << val << std::endl;
  store.Set(ResourceAddress(key), val);
}

void PrintKey(std::string const &key)
{
  std::cout << key << ": ";

  auto doc = store.Get(ResourceAddress(key));
  std::cout << doc.document << std::endl;
}

/**
 * Demonstration of how the store (a dictionary), can commit and revert state
 *
 */
int main()
{

  store.New("a.db", "b.db", "c.db", "d.db");

  {
    std::cout << "=============  BOOK " << book << "  ==================" << std::endl;
    std::cout << "Initial hash:" << std::endl;
    std::cout << byte_array::ToBase64(store.Hash()) << std::endl;

    std::cout << "Keys" << std::endl;
    PrintKey("one");
    PrintKey("two");
    PrintKey("three");

    Set("one", "removed");
    Set("two", "new");
    Set("three", "blasted");
  }

  {
    Set("one", "val");
    Set("two", "thing");
    Set("three", "");

    std::cout << "=============  BOOK " << book << "  ==================" << std::endl;
    std::cout << "Hash:" << std::endl;
    std::cout << byte_array::ToBase64(store.Hash()) << std::endl;

    std::cout << "Keys" << std::endl;
    PrintKey("one");
    PrintKey("two");
    PrintKey("three");
    std::cout << "Commiting " << book << std::endl;
    store.Commit(book);
    book++;
  }

  {
    std::cout << "=============  BOOK " << book << "  ==================" << std::endl;
    std::cout << "Hash:" << std::endl;
    std::cout << byte_array::ToBase64(store.Hash()) << std::endl;

    std::cout << "Keys" << std::endl;
    PrintKey("one");
    PrintKey("two");
    PrintKey("three");
  }

  {
    Set("one", "removed");
    Set("two", "new");
    Set("three", "blasted");

    std::cout << "=============  BOOK " << book << "  ==================" << std::endl;
    std::cout << "Hash:" << std::endl;
    std::cout << byte_array::ToBase64(store.Hash()) << std::endl;

    std::cout << "Keys" << std::endl;
    PrintKey("one");
    PrintKey("two");
    PrintKey("three");
    std::cout << "Commiting " << book << std::endl;
    store.Commit(book);
    book++;
  }

  {
    std::cout << "=============  BOOK " << book << "  ==================" << std::endl;
    std::cout << "Hash:" << std::endl;
    std::cout << byte_array::ToBase64(store.Hash()) << std::endl;

    std::cout << "Reverting to book 1" << std::endl;
    book = 1;
    store.Revert(book);

    std::cout << "Keys" << std::endl;
    PrintKey("one");
    PrintKey("two");
    PrintKey("three");
  }

  return 0;
}
