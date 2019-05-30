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

#include "ledger/identifier.hpp"
#include "core/byte_array/byte_array.hpp"

#include <stdexcept>

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;

static constexpr char const *LOGGING_NAME = "Identifier";

namespace fetch {
namespace ledger {
namespace {

template <std::size_t MIN_LENGTH, std::size_t MAX_LENGTH>
bool IsBase58(ConstByteArray const &value)
{
  char const *buffer = value.char_pointer();

  if (!(value.size() >= MIN_LENGTH && value.size() <= MAX_LENGTH))
  {
    return false;
  }

  for (std::size_t i = 0; i < value.size(); ++i, ++buffer)
  {
    // 1-9A-HJ-NP-Za-km-z
    bool const valid =
        ((('1' <= *buffer) && ('9' >= *buffer)) || (('A' <= *buffer) && ('H' >= *buffer)) ||
         (('J' <= *buffer) && ('N' >= *buffer)) || (('P' <= *buffer) && ('Z' >= *buffer)) ||
         (('a' <= *buffer) && ('k' >= *buffer)) || (('m' <= *buffer) && ('z' >= *buffer)));

    if (!valid)
    {
      return false;
    }
  }

  return true;
}

template <std::size_t LENGTH>
bool IsHex(ConstByteArray const &value)
{
  char const *buffer = value.char_pointer();

  if (LENGTH != value.size())
  {
    return false;
  }

  for (std::size_t i = 0; i < LENGTH; ++i, ++buffer)
  {
    bool const valid =
        ((('a' <= *buffer) && ('f' >= *buffer)) || (('0' <= *buffer) && ('9' >= *buffer)));

    if (!valid)
    {
      return false;
    }
  }

  return true;
}

bool IsDigest(ConstByteArray const &value)
{
  return IsHex<64>(value);
}

bool IsIdentity(ConstByteArray const &value)
{
  return IsBase58<48, 50>(value);
}

}  // namespace

const char Identifier::SEPARATOR{'.'};

/**
 * Construct an identifier from a fully qualified name
 *
 * @param identifier The fully qualified name to parse
 */
Identifier::Identifier(ConstByteArray identifier)
  : full_{std::move(identifier)}
{
  if (!Tokenise())
  {
    throw std::runtime_error("Unable to parse identifier");
  }
}

/**
 * Construct an identifier from another set of tokens
 * @param tokens
 * @param count
 */
Identifier::Identifier(Tokens const &tokens, std::size_t count)
{
  if (count <= tokens.size())
  {
    ByteArray full;

    for (std::size_t i = 0; i < count; ++i)
    {
      // select the current token
      auto const &current_token = tokens[i];

      // add the token to the list
      tokens_.push_back(current_token);

      // regenerate the full name
      if (i)
      {
        full.Append(".");
      }

      full.Append(current_token);
    }

    // regenerate the full name
    full_ = std::move(full);

    // update the type
    UpdateType();
  }
}

/**
 * Get the parent to the identifier
 * @return
 */
Identifier Identifier::GetParent() const
{
  std::size_t const num_tokens = tokens_.size();

  return Identifier{tokens_, (num_tokens) ? (num_tokens - 1) : 0};
}

/**
 * Internal: Break up the fully qualified name into tokens
 */
bool Identifier::Tokenise()
{
  // ensure the tokens storage is empty
  tokens_.clear();

  std::size_t offset = 0;
  for (;;)
  {
    // find the next instance of the separator
    std::size_t const index = full_.Find(SEPARATOR, offset);

    // determine if this is the last token
    bool const last_token = (ConstByteArray::NPOS == index);

    // calculate the size of the element
    std::size_t const size = (last_token) ? (full_.size() - offset) : (index - offset);

    // empty tokens are invalid
    if (size == 0)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Identifier contains empty tokens");
      return false;
    }

    // add the new token to the array
    tokens_.push_back(full_.SubArray(offset, size));

    if (last_token)
    {
      break;
    }

    // update the index
    offset = index + 1;
  }

  // update the type
  UpdateType();

  return true;
}

void Identifier::UpdateType()
{
  // successful parse
  type_ = Type::NORMAL;

  // once the parse is complete decide if this identifier matches that of a smart contract
  if ((1u <= tokens_.size()) && (3u >= tokens_.size()))
  {
    bool is_smart_contract = IsDigest(tokens_[0]);

    if (2u <= tokens_.size())
    {
      is_smart_contract &= IsIdentity(tokens_[1]);
    }

    if (is_smart_contract)
    {
      type_ = Type::SMART_CONTRACT;
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
 * Determine if the current identifier is a child to a specified identifier
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
