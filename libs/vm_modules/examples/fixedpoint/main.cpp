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
#include "vm_modules/core/print.hpp"
#include "vm_modules/core/type_convert.hpp"
#include "vm_modules/math/math.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "usage ./" << argv[0] << " [filename]" << std::endl;
    std::exit(-9);
  }

  // Reading file
  std::ifstream      file(argv[1], std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  auto module = std::make_shared<fetch::vm::Module>();

  fetch::vm_modules::CreatePrint(*module);
  fetch::vm_modules::CreateToString(*module);
  fetch::vm_modules::math::BindMath(*module);

  // Setting compiler up
  fetch::vm::Compiler *    compiler = new fetch::vm::Compiler(module.get());
  fetch::vm::IR            ir;
  fetch::vm::Executable    exec;
  std::vector<std::string> errors;

  // Compiling
  bool compiled = compiler->Compile(source, "myexec", ir, errors);

  if (!compiled)
  {
    std::cout << "Failed to compile" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  // Setting VM up and running
  std::string        error;
  fetch::vm::Variant output;

  fetch::vm::VM vm(module.get());

  // attach std::cout for printing
  vm.AttachOutputDevice("stdout", std::cout);

  if (!vm.GenerateExecutable(ir, "main_ir", exec, errors))
  {
    std::cout << "Failed to generate executable" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  if (!exec.FindFunction("main"))
  {
    std::cout << "Function 'main' not found" << std::endl;
    return -2;
  }

  if (!vm.Execute(exec, "main", error, output))
  {
    std::cout << "Runtime error on line " << error << std::endl;
  }
  delete compiler;
  return 0;
}
