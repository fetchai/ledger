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

struct IntPair : public fetch::vm::Object
{
  IntPair()          = delete;
  virtual ~IntPair() = default;

  IntPair(fetch::vm::VM *vm, fetch::vm::TypeId type_id, int32_t i, int32_t j)
    : fetch::vm::Object(vm, type_id)
    , first_(i)
    , second_(j)
  {}

  static fetch::vm::Ptr<IntPair> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                             int const &i, int const &j)
  {
    return new IntPair(vm, type_id, i, j);
  }

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

static void Print(fetch::vm::VM * /*vm*/, fetch::vm::Ptr<fetch::vm::String> const &s)
{
  std::cout << s->str << std::endl;
}

fetch::vm::Ptr<fetch::vm::String> toString(fetch::vm::VM *vm, int32_t const &a)
{
  fetch::vm::Ptr<fetch::vm::String> ret(new fetch::vm::String(vm, std::to_string(a)));
  return ret;
}

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

  fetch::vm::Module module;

  module.CreateFreeFunction("Print", &Print);
  module.CreateFreeFunction("toString", &toString);

  module.CreateClassType<IntPair>("IntPair")
      .CreateTypeConstuctor<int, int>()
      .CreateInstanceFunction("first", &IntPair::first)
      .CreateInstanceFunction("second", &IntPair::second);

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
  std::string        error;
  fetch::vm::Variant output;

  fetch::vm::VM vm(&module);
  if (!vm.Execute(script, "main", error, output))
  {
    std::cout << "Runtime error on line " << error << std::endl;
  }
  delete compiler;
  return 0;
}
