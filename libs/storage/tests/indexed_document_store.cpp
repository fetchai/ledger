#include<iostream>
#include"storage/indexed_document_store.hpp"
#include"core/byte_array/encoders.hpp"
using namespace fetch;

using namespace fetch::storage;


int main() 
{
  IndexedDocumentStore store;

  store.Load("a.db", "b.db", "c.db", true);
  std::cout << byte_array::ToBase64( store.Hash() ) << std::endl;
  
  {
    auto doc = store.GetDocument("Hello world");
    std::cout << doc.id() << std::endl;
//    doc.Write("Hello world");
  }
  std::cout << byte_array::ToBase64( store.Hash() ) << std::endl;
  
  return 0;
}
