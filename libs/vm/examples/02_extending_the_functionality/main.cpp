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

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include <fstream>
#include <sstream>

struct IntPair
{
  IntPair(int const &i, int const &j)
    : first_(i)
    , second_(j)
  {}

  int first()
  {
    return first_;
  }
  int second()
  {
    return second_;
  }

private:
  int first_;
  int second_;
};

void Print(std::string const &s)
{
  std::cout << s << std::endl;
}

std::string toString(int32_t const &a)
{
  return std::to_string(a);
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cerr << "usage ./" << argv[0] << " [filename]" << std::endl;
    exit(-9);
  }

  // Reading file
  std::ifstream      file(argv[1], std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  fetch::vm::Module module;
  module.ExportClass<IntPair>("IntPair")
      .Constructor<int, int>()
      .Export("first", &IntPair::first)
      .Export("second", &IntPair::second);

  module.ExportFunction("Print", &Print);
  module.ExportFunction("toString", &toString);

  // Setting compiler up
  fetch::vm::Compiler *    compiler = new fetch::vm::Compiler(&module);
  fetch::vm::Script        script;
  std::vector<std::string> errors;

  // Compiling
  bool compiled = compiler->Compile(source, "myscript", script, errors);

  if (!compiled)
  {
    std::cout << "Failed to compile" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }
    return -1;
  }

  if (!script.FindFunction("main"))
  {
    std::cout << "Function 'main' not found" << std::endl;
    return -2;
  }

  // Setting VM up and running
  fetch::vm::VM vm(&module);
  if (!vm.Execute(script, "main"))
  {
    std::cout << "Runtime error on line " << vm.error_line() << ": " << vm.error() << std::endl;
  }
  delete compiler;
  return 0;
}
