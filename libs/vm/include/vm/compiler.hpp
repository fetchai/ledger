#pragma once
#include "vm/analyser.hpp"
#include "vm/generator.hpp"
#include "vm/parser.hpp"

namespace fetch {
namespace vm {

class Compiler
{
public:
  Compiler() {}
  ~Compiler() {}
  bool Compile(const std::string &source, const std::string &name, Script &script,
               std::vector<std::string> &errors);

private:
  Parser    parser_;
  Analyser  analyser_;
  Generator generator_;
};

}  // namespace vm
}  // namespace fetch
