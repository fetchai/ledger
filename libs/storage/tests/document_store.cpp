//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

//#include "core/byte_array/encoders.hpp"
//#include "storage/revertible_document_store.hpp"
//#include <iostream>
//using namespace fetch;
//
//using namespace fetch::storage;
//class TestStore : public RevertibleDocumentStore
//{
//public:
//  typename RevertibleDocumentStore::DocumentFile GetDocumentFile(ResourceID const &rid,
//                                                                 bool const &      create = true)
//  {
//    return RevertibleDocumentStore::GetDocumentFile(rid, create);
//  }
//};
//
//TestStore store;
//
//uint64_t book = 1;
//
//void Add()
//{
//  std::cout << "=============  ADD  ==================" << std::endl;
//  ;
//  {
//    auto doc = store.GetDocumentFile(ResourceID("Hello world"));
//    doc.Seek(doc.size());
//    doc.Write("Hello world");
//  }
//  ++book;
//  store.Commit(book);
//}
//
//void Remove()
//{
//  std::cout << "=============  REMOVE  ==================" << std::endl;
//  ;
//  --book;
//  store.Revert(book);
//}
//
//void Print()
//{
//
//  std::cout << std::endl;
//  auto doc = store.GetDocumentFile(ResourceID("Hello world"));
//  std::cout << "BOOK: " << book << " " << doc.id() << std::endl;
//  std::cout << byte_array::ToBase64(store.Hash()) << std::endl;
//  byte_array::ByteArray x;
//  x.Resize(doc.size());
//  doc.Read(x);
//  std::cout << "VALUE: " << x << std::endl;
//}
//
//int main()
//{
//
//  store.New("a.db", "b.db", "c.db", "d.db");
//  std::cout << byte_array::ToBase64(store.Hash()) << std::endl;
//  char c;
//  Print();
//  Add();
//  Print();
//  Add();
//  Print();
//  Remove();
//  Print();
//  Add();
//  Print();
//  Remove();
//  Print();
//  return 0;
//
//  do
//  {
//    Print();
//
//    std::cin >> c;
//    switch (c)
//    {
//    case 'a':
//    {
//      Add();
//    }
//    break;
//    case 'r':
//      Remove();
//
//      break;
//    }
//
//  } while ((c != 'q') && (std::cin) && (!std::cin.eof()));
//
//  return 0;
//}
