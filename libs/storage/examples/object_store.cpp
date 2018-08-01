#include "storage/object_store.hpp"
#include "core/commandline/cli_header.hpp"
#include <iostream>
using namespace fetch::storage;

int main()
{
  //  fetch::logger.DisableLogger();
  ObjectStore<int> hello;
  hello.Load("fileA.db", "indexA.db");

  int x = 128312;
  int y = 0;
  int z = 0;

  // First run, this will fail. Second run, this will be loaded from file
  bool success = hello.Get(ResourceID("blah"), y);

  std::cout << "success: " << success << std::endl;
  std::cout << y << std::endl;

  hello.Set(ResourceID("blah"), x);
  hello.Get(ResourceID("blah"), z);

  std::cout << z << std::endl;

  return 0;
}
