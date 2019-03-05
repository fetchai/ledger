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

#include "vm_modules/vm_factory.hpp"
#include <fstream>
#include <sstream>

using namespace fetch;
using namespace fetch::vm_modules;

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "usage ./" << argv[0] << " [filename]" << std::endl;
    exit(-9);
  }

  // Reading file
  std::ifstream file(argv[1], std::ios::binary);
  if (!file)
  {
    throw std::runtime_error("Failed to find input file.");
  }
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  auto module = VMFactory::GetModule();

  // ********** Attach way to interface with state here **********
  // module->state

  // Compile source, get runnable script
  fetch::vm::Script        script;
  std::vector<std::string> errors = VMFactory::Compile(module, source, script);

  for (auto const &error : errors)
  {
    std::cerr << error << std::endl;
  }

  if (errors.size() > 0)
  {
    return -1;
  }

  // Execute smart contract
  std::string        error;
  fetch::vm::Variant output;

  // Get clean VM instance
  auto vm = VMFactory::GetVM(module);

  // Execute our fn
  if (!vm->Execute(script, "main", error, output))
  {
    std::cerr << "Runtime error: " << error << std::endl;
  }

  return 0;
}
