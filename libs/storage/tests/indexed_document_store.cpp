#include<iostream>
#include"storage/indexed_document_store.hpp"

using namespace fetch::storage;


int main() 
{
  IndexedDocumentStore store;

  store.Load("a.db", "b.db", "c.db", true);
  
  auto doc = store.GetDocument("Hello world");
  std::cout << doc.id() << std::endl;
//  doc.Write("Hello world");
  
  return 0;
}
