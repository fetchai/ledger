#include "vm/compiler.hpp"
#include "vm/vm.hpp"
#include <fstream>
#include <sstream>

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "usage ./" << argv[0] << " [filename]" << std::endl;
    exit(-9);
  }

  std::ifstream      file(argv[1], std::ios::binary);
  std::ostringstream ss;
  ss << file.rdbuf();
  const std::string source = ss.str();
  file.close();

  fetch::vm::Compiler *compiler = new fetch::vm::Compiler();

  fetch::vm::Script        script;
  std::vector<std::string> errors;
  bool                     compiled = compiler->Compile(source, "myscript", script, errors);

  fetch::vm::VM vm;
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

  vm.Execute(script, "main");
  delete compiler;
  return 0;
}
