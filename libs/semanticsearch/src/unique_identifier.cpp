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

#include "semanticsearch/scope_manager.hpp"
#include "semanticsearch/unique_identifier.hpp"

namespace fetch {
namespace semanticsearch {

UniqueIdentifierPtr UniqueIdentifier::Parse(std::string str_uid, ScopeManagerPtr scope_manager)
{
  if (scope_manager->HasUniqueID(str_uid))
  {
    return scope_manager->GetUniqueID(str_uid);
  }

  // Identifying type
  auto type_pos = str_uid.find(':');
  if (type_pos == std::string::npos)
  {
    return nullptr;
  }

  // Extracting type
  auto type = str_uid.substr(type_pos + 1, str_uid.size() - type_pos - 1);
  Kind kind;
  if (type == "schema")
  {
    kind = SCHEMA;
  }
  else if (type == "instance")
  {
    kind = INSTANCE;
  }
  else if (type == "reducer")
  {
    kind = REDUCER;
  }
  else if (type == "namespace")
  {
    kind = NAMESPACE;
  }
  else
  {
    return nullptr;
  }

  // Extracting autonomic
  Taxonomy    taxonomy;
  std::string taxonomy_string = str_uid.substr(0, type_pos);
  uint64_t    lastpos         = 0;
  uint64_t    pos             = taxonomy_string.find('.', lastpos);

  // Segmenting the UID into components
  while (pos != std::string::npos)
  {
    auto x = str_uid.substr(lastpos, pos - lastpos);
    taxonomy.emplace_back(x);
  }

  // Taking care of the last component
  auto x = str_uid.substr(lastpos, str_uid.size() - lastpos);
  taxonomy.emplace_back(x);

  auto ptr =
      UniqueIdentifierPtr(new UniqueIdentifier(std::move(str_uid), kind, std::move(taxonomy)));
  scope_manager->RegisterUniqueID(ptr);

  return ptr;
}

bool UniqueIdentifier::operator==(UniqueIdentifier const &other) const
{
  return uid_ == other.uid_;
}

bool UniqueIdentifier::operator<=(UniqueIdentifier const &other) const
{
  return uid_ <= other.uid_;
}

bool UniqueIdentifier::operator<(UniqueIdentifier const &other) const
{
  return uid_ < other.uid_;
}

bool UniqueIdentifier::operator>=(UniqueIdentifier const &other) const
{
  return uid_ >= other.uid_;
}

bool UniqueIdentifier::operator>(UniqueIdentifier const &other) const
{
  return uid_ > other.uid_;
}

UniqueIdentifier::Kind UniqueIdentifier::kind() const
{
  return kind_;
}
bool UniqueIdentifier::IsSchema() const
{
  return kind_ == SCHEMA;
}
bool UniqueIdentifier::IsInstance() const
{
  return kind_ == INSTANCE;
}

bool UniqueIdentifier::IsReducer() const
{
  return kind_ == REDUCER;
}

bool UniqueIdentifier::IsNamespace() const
{
  return kind_ == NAMESPACE;
}

UniqueIdentifier::Taxonomy const &UniqueIdentifier::taxonomy() const
{
  return taxonomy_;
}

}  // namespace semanticsearch
}  // namespace fetch
