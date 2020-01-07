#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
    SMART_OR_SYNERGETIC_CONTRACT
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
  bool                  empty() const;

  Identifier GetParent() const;

  // Parsing
  bool Parse(ConstByteArray name);

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

  bool Tokenise(ConstByteArray &&full_name);
  void UpdateType();

  Type           type_{Type::INVALID};
  ConstByteArray full_{};    ///< The fully qualified name
  Tokens         tokens_{};  ///< The individual elements of the name
};

}  // namespace ledger
}  // namespace fetch
