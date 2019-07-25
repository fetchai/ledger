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

#include "core/logging.hpp"
#include "vm/generator.hpp"

namespace fetch {
namespace vm {

enum class FunctionDecoratorKind
{
  NORMAL,   ///< Normal (undecorated) function
  ACTION,   ///< A Transaction handler
  QUERY,    ///< A Query handler
  ON_INIT,  ///< A function to be called on smart contract construction
  INVALID,  ///< The function has an invalid decorator
};

/**
 * Determine the type of the VM function
 *
 * @param fn The reference to function entry
 * @return The type of the function
 */
inline FunctionDecoratorKind DetermineKind(vm::Executable::Function const &fn)
{
  FunctionDecoratorKind kind{FunctionDecoratorKind::NORMAL};

  // loop through all the function annotations
  if (1u == fn.annotations.size())
  {
    // select the first annotation
    auto const &annotation = fn.annotations.front();

    if (annotation.name == "@query")
    {
      // only update the kind if one hasn't already been specified
      kind = FunctionDecoratorKind::QUERY;
    }
    else if (annotation.name == "@action")
    {
      kind = FunctionDecoratorKind::ACTION;
    }
    else if (annotation.name == "@init")
    {
      kind = FunctionDecoratorKind::ON_INIT;
    }
    else
    {
      FETCH_LOG_WARN("function_decorators", "Invalid decorator: ", annotation.name);
      kind = FunctionDecoratorKind::INVALID;
    }
  }

  return kind;
}

}  // namespace vm
}  // namespace fetch
