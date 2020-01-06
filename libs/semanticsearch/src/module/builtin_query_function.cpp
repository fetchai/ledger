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

#include "semanticsearch/module/builtin_query_function.hpp"

#include <cassert>

namespace fetch {
namespace semanticsearch {

bool BuiltinQueryFunction::ValidateSignature(std::type_index const &             ret,
                                             std::vector<std::type_index> const &args)
{
  bool val = (ret == return_type_);
  val      = val && ((args.size() == arguments_.size()));

  if (!val)
  {
    return false;
  }

  for (std::size_t i = 0; i < args.size(); ++i)
  {
    val = val && (args[i] == arguments_[i]);
  }

  return val;
}

QueryVariant BuiltinQueryFunction::operator()(std::vector<void const *> &args)
{
  if (caller_)
  {
    return caller_(args);
  }

  return nullptr;
}

std::type_index BuiltinQueryFunction::return_type() const
{
  return return_type_;
}

}  // namespace semanticsearch
}  // namespace fetch
