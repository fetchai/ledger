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

namespace fetch {
namespace vm {

enum class Kind
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
inline Kind DetermineKind(vm::Script::Function const &fn)
{
  Kind kind{Kind::NORMAL};

  // loop through all the function annotations
  if (1u == fn.annotations.size())
  {
    // select the first annotation
    auto const &annotation = fn.annotations.front();

    if (annotation.name == "@query")
    {
      // only update the kind if one hasn't already been specified
      kind = Kind::QUERY;
    }
    else if (annotation.name == "@action")
    {
      kind = Kind::ACTION;
    }
    else if (annotation.name == "@on_init")
    {
      kind = Kind::ON_INIT;
    }
    else
    {
      FETCH_LOG_WARN("function_decorators", "Invalid decorator: ", annotation.name);
      kind = Kind::INVALID;
    }
  }

  return kind;
}

}  // namespace vm
}  // namespace fetch
