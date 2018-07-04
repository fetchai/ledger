#include<iostream>
#include"storage/object_store.hpp"
#include"core/commandline/cli_header.hpp"
using namespace fetch::storage;

int main() 
{
//  fetch::logger.DisableLogger();
  ObjectStore< int > hello;
  hello.Load("a.db", "b.db");

  int x = 128312, y;
  hello.Set(ResourceID("blah"), x);
  hello.Get(ResourceID("blah"), y);

  std::cout << y << std::endl;
  
  return 0;
}
