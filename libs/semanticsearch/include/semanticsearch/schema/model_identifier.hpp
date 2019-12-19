#pragma once
#include "semanticsearch/index/base_types.hpp"
#include "semanticsearch/schema/scope_identifier.hpp"

#include <ostream>
#include <sstream>

namespace fetch {
namespace semanticsearch {

struct ModelIdentifier
{
  ScopeIdentifier scope{};
  std::string     model_name{""};

  operator std::string() const;

  bool operator<(ModelIdentifier const &other) const
  {
    if (scope == other.scope)
    {
      return model_name < other.model_name;
    }

    return scope < other.scope;
  }

  bool operator==(ModelIdentifier const &other) const
  {
    return (scope == other.scope) && (model_name == other.model_name);
  }
};

inline std::ostream &operator<<(std::ostream &s, ModelIdentifier const &n)
{
  s << n.scope.address << "@" << n.scope.version << ":" << n.model_name;
  return s;
}

inline ModelIdentifier::operator std::string() const
{
  std::stringstream ss{""};
  ss << *this;
  return ss.str();
}

}  // namespace semanticsearch
}  // namespace fetch