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

#include "vm/module.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void Print(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  std::cout << s->str << std::endl;
}

fetch::vm::Ptr<fetch::vm::String> toString(fetch::vm::VM *vm, int32_t const &a)
{
  fetch::vm::Ptr<fetch::vm::String> ret(new fetch::vm::String(vm, std::to_string(a)));
  return ret;
}

struct System : public fetch::vm::Object
{
  System()           = delete;
  ~System() override = default;

  static int32_t Argc(fetch::vm::VM * /*vm*/, fetch::vm::TypeId /*type_id*/)
  {
    return int32_t(System::args.size());
  }

  static fetch::vm::Ptr<fetch::vm::String> Argv(fetch::vm::VM *vm, fetch::vm::TypeId /*type_id*/,
                                                int32_t const &a)
  {
    return fetch::vm::Ptr<fetch::vm::String>(
        new fetch::vm::String(vm, System::args[std::size_t(a)]));
  }

  static std::vector<std::string> args;
};

std::vector<std::string> System::args;

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "usage ./" << argv[0] << " [filename]" << std::endl;
    std::exit(-9);
  }

  for (int i = 1; i < argc; ++i)
  {
    System::args.emplace_back(argv[i]);
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

  // Creating new VM module
  fetch::vm::Module module;

  module.CreateFreeFunction("print", &Print);
  module.CreateFreeFunction("toString", &toString);
  module.CreateClassType<System>("System")
      .CreateStaticMemberFunction("Argc", &System::Argc)
      .CreateStaticMemberFunction("Argv", &System::Argv);

  // Setting compiler up
  auto compiler = std::make_unique<fetch::vm::Compiler>(&module);
  auto vm       = std::make_unique<fetch::vm::VM>(&module);

  fetch::vm::Executable    executable;
  fetch::vm::IR            ir;
  std::vector<std::string> errors;
  bool                     compiled = compiler->Compile(source, "myexecutable", ir, errors);

  if (!vm->GenerateExecutable(ir, "main_ir", executable, errors))
  {
    std::cout << "Failed to generate executable" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  if (!compiled)
  {
    std::cout << "Failed to compile" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  if (!executable.FindFunction("main"))
  {
    std::cout << "Function 'main' not found" << std::endl;
    return -2;
  }

  std::string        error;
  std::string        console;
  fetch::vm::Variant output;

  // Setting VM up and running
  if (!vm->Execute(executable, "main", error, output))
  {
    std::cout << "Runtime error: " << error << std::endl;
  }

  return 0;
}
