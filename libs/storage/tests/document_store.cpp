#include<iostream>
#include"storage/revertible_document_store.hpp"
#include"core/byte_array/encoders.hpp"
using namespace fetch;

using namespace fetch::storage;
class TestStore: public RevertibleDocumentStore 
{
public:
  typename RevertibleDocumentStore::DocumentFile GetDocumentFile(ResourceID const &rid,  bool const &create = true) 
  {
    return RevertibleDocumentStore::GetDocumentFile(rid, create);
  }
 
};

TestStore store;


uint64_t book = 1;

void Add() 
{
  std::cout << "=============  ADD  ==================" << std::endl;;  
  {        
    auto doc = store.GetDocumentFile( ResourceID("Hello world") );
    doc.Seek( doc.size() );
    doc.Write("Hello world");
  }
  ++book;
  store.Commit(book);  
}

void Remove() 
{
  std::cout << "=============  REMOVE  ==================" << std::endl;;    
  --book;      
  store.Revert(book);

}

void Print() 
{
  
  std::cout << std::endl;
  auto doc = store.GetDocumentFile( ResourceID("Hello world") );
  std::cout << "BOOK: " <<  book << " " << doc.id() << std::endl;
  std::cout << byte_array::ToBase64( store.Hash() ) << std::endl;
  byte_array::ByteArray x;
  x.Resize(doc.size());
  doc.Read(x);
  std::cout << "VALUE: " << x << std::endl;
  

}


int main() 
{


  store.New("a.db", "b.db", "c.db", "d.db");
  std::cout << byte_array::ToBase64( store.Hash() ) << std::endl;
  char c;
  Print();
  Add();
  Print();
  Add();
  Print();
  Remove();
  Print();
  Add();
  Print();
  Remove();
  Print();
  return 0;
  
  
  do {
    Print();

    std::cin >> c;    
    switch(c) {
    case 'a':      
    {
      Add();
    }
    break;
    case 'r': 
      Remove();
      
    break;
    }
    
  } while ((c !='q') && (std::cin) && (!std::cin.eof()));
  

  
  return 0;
}
