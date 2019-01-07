#pragma once
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

#include <core/byte_array/byte_array.hpp>
#include <vector>

namespace fetch {
namespace ledger {

/**
 * A string identifier which is seperated with '.' used as to hierarchically
 * group related objects.
 *
 * For example:
 *
 *   `foo.bar` & `foo.baz` are in the same logic `foo` group
 */
  // TODO(HUT): chat to Ed
class Identifier
{
public:
  using string_type = byte_array::ConstByteArray;
  using tokens_type = std::vector<string_type>;

  static constexpr char SEPARATOR = '.';

  // Construction / Destruction
  Identifier() = default;
  explicit Identifier(string_type identifier);
  Identifier(Identifier const &) = default;
  Identifier(Identifier &&)      = default;
  ~Identifier()                  = default;

  // Accessors
  string_type        name() const;
  string_type        name_space() const;
  string_type const &full_name() const;

  // Parsing
  void Parse(string_type const &name);
  void Parse(string_type &&name);

  // Comparison
  bool IsParentTo(Identifier const &other) const;
  bool IsChildTo(Identifier const &other) const;
  bool IsDirectParentTo(Identifier const &other) const;
  bool IsDirectChildTo(Identifier const &other) const;

  void Append(string_type const &element);

  // Operators
  Identifier &       operator=(Identifier const &) = default;
  Identifier &       operator=(Identifier &&) = default;
  string_type const &operator[](std::size_t index) const;
  bool               operator==(Identifier const &other) const;
  bool               operator!=(Identifier const &other) const;

private:
  byte_array::ByteArray    full_{};    ///< The fully qualified name
  tokens_type              tokens_{};  ///< The individual elements of the name
  static string_type const separator_;

  void Tokenise();
};

/**
 * Gets the top level name i.e. in the case of `foo.bar` `bar` would be
 * returned
 *
 * @return the top level name or an empty string if the identifier is empty
 */
inline Identifier::string_type Identifier::name() const
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
inline Identifier::string_type Identifier::name_space() const
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
inline Identifier::string_type const &Identifier::full_name() const
{
  return full_;
}

/**
 * Parses an fully qualified name
 *
 * @param name The fully qualified name
 */
inline void Identifier::Parse(string_type const &name)
{
  full_ = name;
  Tokenise();
}

/**
 * Parse a fully qualified name
 *
 * @param name The fully qualified name
 */
inline void Identifier::Parse(string_type &&name)
{
  full_ = std::move(name);
  Tokenise();
}

/**
 * Access elements of the name.
 *
 * @param index The index to be accessed
 * @return The element of the name
 */
inline Identifier::string_type const &Identifier::operator[](std::size_t index) const
{
#ifndef NDEBUG
  return tokens_[index];
#else   // !NDEBUG
  return tokens_.at(index);
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

/**
 * Append an element to a name
 *
 * @param element The element to be added to the name
 */
inline void Identifier::Append(string_type const &element)
{
  Identifier id_to_add{element};

  if (full_.size() > 0)
  {
    full_.Append(separator_, id_to_add.full_name());
  }
  else
  {
    full_.Append(id_to_add.full_name());
  }

  tokens_.insert(tokens_.end(), id_to_add.tokens_.begin(), id_to_add.tokens_.end());
}

}  // namespace ledger
}  // namespace fetch
