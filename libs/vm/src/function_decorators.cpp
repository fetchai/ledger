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

#include "logging/logging.hpp"
#include "vm/function_decorators.hpp"
#include "vm/generator.hpp"

namespace fetch {
namespace vm {

FunctionDecoratorKind DetermineKind(vm::Executable::Function const &fn)
{
  if (fn.annotations.empty())
  {
    return FunctionDecoratorKind::NONE;
  }

  FunctionDecoratorKind kind{FunctionDecoratorKind::NONE};

  // loop through all the function annotations
  if (1u == fn.annotations.size())
  {
    // select the first annotation
    auto const &annotation = fn.annotations.front();

    if (annotation.name == "@query")
    {
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
  else
  {
    std::ostringstream oss;
    for (auto const &annotation : fn.annotations)
    {
      oss << " " << annotation.name;
    }
    FETCH_LOG_WARN("function_decorators", "Multiple decorators found:", oss.str());
    kind = FunctionDecoratorKind::INVALID;
  }

  return kind;
}

}  // namespace vm
}  // namespace fetch
