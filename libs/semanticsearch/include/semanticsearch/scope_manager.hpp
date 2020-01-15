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

#include "semanticsearch/unique_identifier.hpp"

#include <memory>
#include <unordered_map>

namespace fetch {
namespace semanticsearch {

class ScopeManager;
using ScopeManagerPtr = std::shared_ptr<ScopeManager>;

class ScopeManager : std::enable_shared_from_this<ScopeManager>
{
public:
  using UniqueIdentifierPtr = std::shared_ptr<UniqueIdentifier>;
  using TypeId              = UniqueIdentifier::TypeId;
  using TypeIdToUIDMap      = std::unordered_map<TypeId, UniqueIdentifierPtr>;

  /// @{
  static ScopeManagerPtr New(ScopeManagerPtr const &parent = nullptr);
  ScopeManager() = delete;
  /// @}

  /// Id management
  /// @{
  bool                HasUniqueID(std::string const uid) const;
  UniqueIdentifierPtr GetUniqueID(std::string const uid) const;
  void                RegisterUniqueID(UniqueIdentifierPtr ptr);
  /// @}

  /// Execution scope
  /// @{
  ScopeManagerPtr NewScope();
  /// @}
private:
  explicit ScopeManager(ScopeManagerPtr const &parent);

  /// ID registration
  /// @{
  ScopeManagerPtr parent_{nullptr};
  TypeIdToUIDMap  uids_{};
  /// @}
};

}  // namespace semanticsearch
}  // namespace fetch
