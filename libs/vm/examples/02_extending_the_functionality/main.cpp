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

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include <fstream>
#include <sstream>

/*
struct TestClass
{
  TestClass(int const &x) : x_(x) {}

  void Foo() { std::cout << "FOO: " << x_ << std::endl; }

  void Bar(int const &x)
  {  // TOODO: Add support for const
    std::cout << "BAR: " << x << " / " << x_ << std::endl;
  }

  int Baz(int x)
  {
    std::cout << "Baz" << x << " / " << x_ << std::endl;
    return x + x_;
  }

  static int32_t Blah(double x)
  {
    std::cout << "Hello: " << x << std::endl;
    return int(x);
  }

  static double Blah(int32_t x)
  {
    std::cout << "Hello 2: " << x << std::endl;
    return double(x);
  }

private:
  int x_;
};

double TestFunction(int const &x, int const &y)
{
  std::cout << "Add:" << x + y << std::endl;
  return double(x) + double(y);
}

int TestFunction(double const &x, double const &y)
{
  std::cout << "Add 2:" << x + y << std::endl;
  return int(x) + int(y);
}
*/

void Print(std::string const &s)
{
  std::cout << s << std::endl;
}

struct System
{
  static int32_t Argc()
  {
    return int32_t(System::args.size());
  }

  static std::string Argv(int32_t const &a)
  {
    return System::args[std::size_t(a)];
  }

  static std::vector<std::string> args;
};

std::vector<std::string> System::args;

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

  for (int i = 1; i < argc; ++i)
  {
    System::args.push_back(std::string(argv[i]));
  }

  // Reading file
  std::ifstream      file(argv[1], std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  fetch::vm::Module module;
  module.ExportFunction("Print", &Print);
  module.ExportFunction("toString", &toString);

  module.ExportClass<System>("System")
      .ExportStaticFunction("Argc", System::Argc)
      .ExportStaticFunction("Argv", System::Argv);

  // Creating new VM module
  /*
  module.ExportClass<TestClass>("TestClass")
      .Constructor<int>()
      .Export("Foo", &TestClass::Foo)
      .Export("Bar", &TestClass::Bar)
      .Export("Baz", &TestClass::Baz)

  module.ExportFunction("AddNumbers", (double (*)(int const &, int const &))TestFunction);
  module.ExportFunction("AddNumbers", (int (*)(double const &, double const &))TestFunction);
*/
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
