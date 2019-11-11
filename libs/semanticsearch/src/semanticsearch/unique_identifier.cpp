#include "semanticsearch/unique_identifier.hpp"
#include "semanticsearch/scope_manager.hpp"

namespace fetch {
namespace semanticsearch {

UniqueIdentifier::UniqueIdentifierPtr UniqueIdentifier::Parse(std::string     str_uid,
                                                              ScopeManagerPtr scope_manager)
{
  if (scope_manager->HasUniqueUID(str_uid))
  {
    return scope_manager->GetUniqueUID(str_uid);
  }

  // Identifying type
  auto type_pos = str_uid.find(':');
  if (type_pos == std::string::npos)
  {
    return nullptr;
  }

  // Extracting type
  auto type = uid.substr(type_pos + 1, str_uid.size() - type_pos - 1);
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

  auto ptr = std::make_shared<UniqueIdentifier>(std::move(str_uid), kind, std::move(taxonomy));
  scope_manager->RegisterUID(ptr);

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

Kind UniqueIdentifier::kind() const
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