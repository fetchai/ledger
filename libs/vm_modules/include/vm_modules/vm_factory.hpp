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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace vm {

class Module;
struct Executable;

}  // namespace vm

namespace vm_modules {

/**
 * VMFactory provides the user with convenient management of the VM
 * and its associated bindings
 */
class VMFactory
{
public:
  using Errors    = std::vector<std::string>;
  using ModulePtr = std::shared_ptr<vm::Module>;

  enum Modules : uint64_t
  {
    MOD_CORE = (1ull << 0ull),
    MOD_MATH = (1ull << 1ull),
    MOD_SYN  = (1ull << 2ull),
    MOD_ML   = (1ull << 3ull),
  };

  enum UseCases : uint64_t
  {
    USE_SMART_CONTRACTS = (MOD_CORE | MOD_MATH | MOD_ML),
    USE_SYNERGETIC      = (MOD_CORE | MOD_MATH | MOD_SYN),
    USE_ALL             = (~uint64_t(0)),
  };

  /**
   * Get a module, the VMFactory will add whatever bindings etc. are considered in the 'standard
   * library'
   *
   * @return: The module
   */
  static std::shared_ptr<fetch::vm::Module> GetModule(uint64_t enabled);

  /**
   * Compile a source file, producing  an executable
   *
   * @param: module The module which the user might have added various bindings/classes to etc.
   * @param: source The raw source to compile
   * @param: executable executable to fill
   *
   * @return: Vector of strings which represent errors found during compilation
   */
  static std::vector<std::string> Compile(std::shared_ptr<fetch::vm::Module> const &module,
                                          std::string const &                       source,
                                          fetch::vm::Executable &                   executable);
};

}  // namespace vm_modules
}  // namespace fetch
