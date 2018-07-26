#include "ledger/identifier.hpp"

#include <stdexcept>

namespace fetch {
namespace ledger {

/**
 * Construct an identifier from a fully qualified name
 *
 * @param identifier The fully qualified name to parse
 */
Identifier::Identifier(std::string identifier) : full_{identifier} { Tokenise(); }

/**
 * Internal: Break up the fully qualified name into tokens
 */
void Identifier::Tokenise()
{
  tokens_.clear();

  std::size_t offset = 0;
  for (;;)
  {
    std::size_t index = full_.find(SEPERATOR, offset);

    if (index == std::string::npos)
    {
      tokens_.push_back(full_.substr(offset));
      break;
    }
    else
    {
      tokens_.push_back(full_.substr(offset, index - offset));
      offset = index + 1;
    }
  }
}

/**
 * Determine if the current identifier is a parent to a specified identifier
 *
 * @param other The prospective child identifier
 * @return true if it is a parent, otherwise false
 */
bool Identifier::IsParentTo(Identifier const &other) const
{
  if (!tokens_.empty() && !other.tokens_.empty())
  {
    if (tokens_.size() < other.tokens_.size())
    {
      return tokens_[0] == other.tokens_[0];
    }
  }
  return false;
}

/**
 * Determmine if the current identifier is a child to a specified identifier
 *
 * @param other The prospective parent identifier
 * @return true if it is a parent, otherwise false
 */
bool Identifier::IsChildTo(Identifier const &other) const { return other.IsParentTo(*this); }

/**
 * Determine if the current identifier is a direct parent to a specified
 * identifier
 *
 * @param other The prospective child identifier
 * @return true if it is a direct parent, otherwise false
 */
bool Identifier::IsDirectParentTo(Identifier const &other) const
{
  bool is_parent{false};

  if ((tokens_.size() + 1) == other.tokens_.size())
  {
    is_parent = true;
    for (std::size_t i = 0; i < tokens_.size(); ++i)
    {
      if (tokens_[i] != other.tokens_[i])
      {
        is_parent = false;
        break;
      }
    }
  }

  return is_parent;
}

/**
 * Determine if the current identifier is a direct child to a specified
 * identifier
 *
 * @param other The prospective parent identifier
 * @return true if is a direct child, otherwise false
 */
bool Identifier::IsDirectChildTo(Identifier const &other) const
{
  return other.IsDirectParentTo(*this);
}

}  // namespace ledger
}  // namespace fetch
