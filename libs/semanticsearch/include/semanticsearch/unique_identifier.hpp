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

#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace semanticsearch {

class ScopeManager;

class UniqueIdentifier;
using UniqueIdentifierPtr = std::shared_ptr<UniqueIdentifier>;

class UniqueIdentifier
{
public:
  using Taxonomy = std::vector<std::string>;

  using TypeId          = uint64_t;
  using ScopeManagerPtr = std::shared_ptr<ScopeManager>;

  enum Kind
  {
    SCHEMA,
    INSTANCE,
    REDUCER,
    NAMESPACE
  };

  UniqueIdentifierPtr Parse(std::string str_uid, ScopeManagerPtr scope_manager);

  /// Comparison
  /// @{
  bool operator==(UniqueIdentifier const &other) const;
  bool operator<=(UniqueIdentifier const &other) const;
  bool operator<(UniqueIdentifier const &other) const;
  bool operator>=(UniqueIdentifier const &other) const;
  bool operator>(UniqueIdentifier const &other) const;
  /// @}

  /// Kind
  /// @{
  Kind kind() const;
  bool IsSchema() const;
  bool IsInstance() const;
  bool IsReducer() const;
  bool IsNamespace() const;
  /// @}

  Taxonomy const &taxonomy() const;

protected:
  void SetID(TypeId uid)
  {
    uid_ = uid;
  }

private:
  UniqueIdentifier(std::string str_uid, Kind kind, Taxonomy taxonomy)
    : str_uid_{std::move(str_uid)}
    , kind_{kind}
    , taxonomy_{std::move(taxonomy)}
  {}

  TypeId      uid_;
  std::string str_uid_;
  Kind        kind_;
  Taxonomy    taxonomy_;

  friend class ScopeManager;
};

}  // namespace semanticsearch
}  // namespace fetch
