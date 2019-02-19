#pragma once
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

#include "vm/free_functions.hpp"

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

/**
 * VMFactory provides the user with convenient management of the VM
 * and its associated bindings
 *
 */
class VMFactory
{
public:
  /**
   * Get a module, the VMFactory will add whatever bindings etc. are considered in the 'standard
   * library'
   *
   * @return: The module
   */
  static std::shared_ptr<fetch::vm::Module> GetModule()
  {
    auto module = std::make_shared<fetch::vm::Module>();

    // Bind our vm free functions to the module
    module->CreateFreeFunction("Print", &Print);
    module->CreateFreeFunction("toString", &toString);

    return module;
  }

  /**
   * Compile a source file, returning a script
   *
   * @param: module The module which the user might have added various bindings/classes to etc.
   * @param: source The raw source to compile
   * @param: script script to fill
   *
   * @return: Vector of strings which represent errors found during compilation
   */
  static std::vector<std::string> Compile(std::shared_ptr<fetch::vm::Module> module,
                                          std::string const &source, fetch::vm::Script &script)
  {
    std::vector<std::string> errors;
    auto                     compiler = std::make_shared<fetch::vm::Compiler>(module.get());

    bool compiled = compiler->Compile(source, "myscript", script, errors);

    if (!script.FindFunction("main"))
    {
      errors.push_back("Function 'main' not found");
    }

    if (!compiled)
    {
      errors.push_back("Failed to compile.");
    }

    return errors;
  }

  /**
   * Get an instance of the VM after binding to a module
   *
   * @param: module Module which the user has added bindings to
   *
   * @return: An instance of the VM
   */
  static std::unique_ptr<fetch::vm::VM> GetVM(std::shared_ptr<fetch::vm::Module> module)
  {
    return std::make_unique<fetch::vm::VM>(module.get());
  }
};

}  // namespace vm
}  // namespace fetch
