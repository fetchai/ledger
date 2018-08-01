#include "storage/object_store.hpp"
#include "core/commandline/cli_header.hpp"
#include <iostream>
using namespace fetch::storage;

int main()
{
  /*
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

  std::cout << z << std::endl; */

  ObjectStore<int> hello;
  hello.Load("fileA.db", "indexA.db");

  int a = 0;
  int b = 0;
  int c = 0;

  hello.Set(ResourceID("blaha"), int(111));
  hello.Set(ResourceID("bther"), int(222));

  fetch::byte_array::ByteArray array;
  array.Resize(256/8);

  for (std::size_t i = 0; i < 256; ++i)
  {
    array[i] = 1;
  }

  hello.Get(ResourceID(array), a);
  hello.Get(ResourceID("blaha"), b);
  hello.Get(ResourceID("bther"), c);

  std::cout << "***" <<  std::dec << a << std::endl;
  std::cout << "***" <<  std::dec << b << std::endl;
  std::cout << "***" <<  std::dec << c << std::endl;

  return 0;
}
