#include "semanticsearch/query/variant.hpp"

namespace fetch {
namespace semanticsearch {

void AbstractQueryVariant::SetType(int type)
{
  type_ = std::move(type);
}

int AbstractQueryVariant::type() const
{
  return type_;
}

void AbstractQueryVariant::SetToken(Token token)
{
  token_ = std::move(token);
}

AbstractQueryVariant::Token AbstractQueryVariant::token() const
{
  return token_;
}

std::type_index AbstractQueryVariant::type_index() const
{
  return type_index_;
}

}  // namespace semanticsearch
}  // namespace fetch