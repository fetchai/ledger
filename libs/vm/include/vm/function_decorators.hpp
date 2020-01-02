#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "vm/generator.hpp"

namespace fetch {
namespace vm {

enum class FunctionDecoratorKind
{
  NONE,       ///< Normal (undecorated) function
  WORK,       ///< A function that is called to do synergetic work
  OBJECTIVE,  ///< A function that is called to determine the quality of synergetic work
  PROBLEM,    ///< The synergetic problem function
  CLEAR,      ///< The synergetic clear function
  ACTION,     ///< A Transaction handler
  QUERY,      ///< A Query handler
  ON_INIT,    ///< A function to be called on smart contract construction
  INVALID,    ///< The function has an invalid decorator
};

/**
 * Determine the type of the VM function
 *
 * @param fn The reference to function entry
 * @return The type of the function
 */
FunctionDecoratorKind DetermineKind(vm::Executable::Function const &fn);

}  // namespace vm
}  // namespace fetch
