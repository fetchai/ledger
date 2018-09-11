//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "ledger/identifier.hpp"

#include <stdexcept>

namespace fetch {
namespace ledger {

constexpr char Identifier::SEPERATOR;
byte_array::ConstByteArray const Identifier::separator_{reinterpret_cast<byte_array::ConstByteArray::container_type const*>(&Identifier::SEPERATOR), 1};

/**
 * Construct an identifier from a fully qualified name
 *
 * @param identifier The fully qualified name to parse
 */
Identifier::Identifier(string_type identifier)
  : full_{std::move(identifier)}
{
  Tokenise();
}

/**
 * Internal: Break up the fully qualified name into tokens
 */
void Identifier::Tokenise()
{
  tokens_.clear();

  std::size_t offset = 0;
  for (;;)
  {
    std::size_t index = full_.Find(SEPERATOR, offset);

    if (index == string_type::NPOS)
    {
      tokens_.push_back(full_.SubArray(offset, full_.size() - offset));
      break;
    }
    else
    {
      tokens_.push_back(full_.SubArray(offset, index - offset));
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
bool Identifier::IsChildTo(Identifier const &other) const
{
  return other.IsParentTo(*this);
}

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
