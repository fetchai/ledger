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

#include "vm/compiler.hpp"
#include "vm/module.hpp"

#include <string>
#include <vector>

namespace fetch {
namespace vm {

Compiler::Compiler(Module *module)
{
  analyser_.Initialise();
  module->CompilerSetup(this);
}

Compiler::~Compiler()
{
  analyser_.UnInitialise();
}

bool Compiler::Compile(std::string const &source, std::string const &name, IR &ir,
                       std::vector<std::string> &errors)
{
  std::string  filename;
  BlockNodePtr root = parser_.Parse(filename, source, errors);

  if (root == nullptr)
  {
    return false;
  }

  bool success = analyser_.Analyse(root, errors);
  if (success)
  {
    builder_.Build(name, root, ir);
  }

  root->Reset();

  return success;
}

}  // namespace vm
}  // namespace fetch
