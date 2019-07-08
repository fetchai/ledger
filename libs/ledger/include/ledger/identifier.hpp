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

#include "core/byte_array/byte_array.hpp"

#include <vector>

namespace fetch {
namespace ledger {

/**
 * A string identifier which is related to the piece of chain code or a smart contract. In general,
 * this is represented by a series of tokens separated with the '.' character
 *
 * For example:
 *
 *   `foo.bar` & `foo.baz` are in the same logical `foo` group
 */
class Identifier
{
public:
  enum class Type
  {
    INVALID,
    NORMAL,
    SMART_CONTRACT
  };

  using ConstByteArray = byte_array::ConstByteArray;
  using Tokens         = std::vector<ConstByteArray>;

  // Construction / Destruction
  Identifier() = default;
  explicit Identifier(ConstByteArray identifier);
  Identifier(Identifier const &) = default;
  Identifier(Identifier &&)      = default;
  ~Identifier()                  = default;

  // Accessors
  Type                  type() const;
  ConstByteArray        name() const;
  ConstByteArray        name_space() const;
  ConstByteArray const &full_name() const;
  ConstByteArray        qualifier() const;
  std::size_t           size() const;
  bool                  empty() const;

  Identifier GetParent() const;

  // Parsing
  bool Parse(ConstByteArray const &name);
  bool Parse(ConstByteArray &&name);

  // Comparison
  bool IsParentTo(Identifier const &other) const;
  bool IsChildTo(Identifier const &other) const;
  bool IsDirectParentTo(Identifier const &other) const;
  bool IsDirectChildTo(Identifier const &other) const;

  // Operators
  Identifier &          operator=(Identifier const &) = default;
  Identifier &          operator=(Identifier &&) = default;
  ConstByteArray const &operator[](std::size_t index) const;
  bool                  operator==(Identifier const &other) const;
  bool                  operator!=(Identifier const &other) const;

private:
  Identifier(Tokens const &tokens, std::size_t count);

  static char const SEPARATOR;

  bool Tokenise();
  void UpdateType();

  Type           type_{Type::INVALID};
  ConstByteArray full_{};    ///< The fully qualified name
  Tokens         tokens_{};  ///< The individual elements of the name
};

/**
 * Gets the current type of the identifier
 *
 * @return The current type
 */
inline Identifier::Type Identifier::type() const
{
  return type_;
}

/**
 * Gets the top level name i.e. in the case of `foo.bar` `bar` would be
 * returned
 *
 * @return the top level name or an empty string if the identifier is empty
 */
inline Identifier::ConstByteArray Identifier::name() const
{
  if (tokens_.empty())
  {
    return {};
  }
  else
  {
    return tokens_.back();
  }
}

/**
 * Gets the namespace for the identifier i.e. in the case of `foo.bar.baz`
 * `foo.bar` would be returned
 *
 * @return The namespace for the identifier
 */
inline Identifier::ConstByteArray Identifier::name_space() const
{
  if (tokens_.size() >= 2)
  {
    return full_.SubArray(0, full_.size() - (tokens_.back().size() + 1));
  }
  else
  {
    return {};
  }
}

/**
 * Gets the fully qualified resource name
 *
 * @return The fully qualified name
 */
inline Identifier::ConstByteArray const &Identifier::full_name() const
{
  return full_;
}

/**
 * Get the unique qualifier for this identifier
 *
 * @return
 */
inline Identifier::ConstByteArray Identifier::qualifier() const
{
  ConstByteArray identifier{};

  switch (type_)
  {
  case Type::INVALID:
    break;
  case Type::NORMAL:
    identifier = full_name();
    break;
  case Type::SMART_CONTRACT:
    identifier = tokens_[0];
    break;
  }

  return identifier;
}

inline std::size_t Identifier::size() const
{
  return tokens_.size();
}

inline bool Identifier::empty() const
{
  return tokens_.empty();
}

/**
 * Parses an fully qualified name
 *
 * @param name The fully qualified name
 */
inline bool Identifier::Parse(ConstByteArray const &name)
{
  full_ = name;
  return Tokenise();
}

/**
 * Parse a fully qualified name
 *
 * @param name The fully qualified name
 */
inline bool Identifier::Parse(ConstByteArray &&name)
{
  full_ = std::move(name);
  return Tokenise();
}

/**
 * Access elements of the name.
 *
 * @param index The index to be accessed
 * @return The element of the name
 */
inline Identifier::ConstByteArray const &Identifier::operator[](std::size_t index) const
{
#ifndef NDEBUG
  return tokens_.at(index);
#else   // !NDEBUG
  return tokens_[index];
#endif  // NDEBUG
}

/**
 * Equality operator
 *
 * @param other The reference to the other identifier
 * @return true if both identifiers are the same, otherwise false
 */
inline bool Identifier::operator==(Identifier const &other) const
{
  return (full_ == other.full_);
}

/**
 * Inequality operator
 *
 * @param other The reference to the other identifier
 * @return true if identifiers are not the same, otherwise false
 */
inline bool Identifier::operator!=(Identifier const &other) const
{
  return (full_ != other.full_);
}

}  // namespace ledger
}  // namespace fetch
