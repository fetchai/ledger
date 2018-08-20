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
