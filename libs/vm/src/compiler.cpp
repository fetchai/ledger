#include "vm/compiler.hpp"
#include <sstream>

namespace fetch {
namespace vm {

bool Compiler::Compile(const std::string &source, const std::string &name, Script &script,
                       std::vector<std::string> &errors)
{
  BlockNodePtr root = parser_.Parse(source, errors);
  if (root == nullptr) return false;

  bool analysed = analyser.Analyse(root, errors);
  if (analysed == false) return false;

  generator_.Generate(root, name, script);

  return true;
}

}  // namespace vm
}  // namespace fetch
