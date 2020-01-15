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

#include "semanticsearch/scope_manager.hpp"

namespace fetch {
namespace semanticsearch {

ScopeManagerPtr ScopeManager::New(ScopeManagerPtr const &parent)
{
  return ScopeManagerPtr(new ScopeManager(parent));
}

ScopeManager::ScopeManager(ScopeManagerPtr const &parent)
  : parent_{parent}
{}

bool ScopeManager::HasUniqueID(std::string const /*uid*/) const
{
  return false;
}

UniqueIdentifierPtr ScopeManager::GetUniqueID(std::string const /*uid*/) const
{
  return {};
}

void ScopeManager::RegisterUniqueID(UniqueIdentifierPtr /*ptr*/)
{}

ScopeManagerPtr ScopeManager::NewScope()
{
  return {};
}

}  // namespace semanticsearch
}  // namespace fetch
