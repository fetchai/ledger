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
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"

#include <memory>
#include <stdexcept>
#include <typeindex>

namespace fetch {
namespace semanticsearch {

class AbstractQueryVariant
{
public:
  using Token = fetch::byte_array::Token;

  virtual ~AbstractQueryVariant()  = default;
  virtual void const *data() const = 0;

  void            SetType(int type);
  int             type() const;
  void            SetToken(Token token);
  Token           token() const;
  std::type_index type_index() const;

  template <typename T>
  explicit operator T() const
  {
    if (std::type_index(typeid(T)) != type_index_)
    {
      throw std::runtime_error("Type mismatch in QueryVariant.");
    }
    return *reinterpret_cast<T *>(data());
  }

  template <typename T>
  bool IsType() const
  {
    return std::type_index(typeid(T)) == type_index_;
  }

  template <typename T>
  T const &As() const
  {
    if (std::type_index(typeid(T)) != type_index_)
    {
      auto t = std::type_index(typeid(T));
      throw std::runtime_error("Type mismatch in QueryVariant: stored " +
                               std::string(type_index_.name()) + " vs. requested " +
                               std::string(t.name()));
    }
    return *reinterpret_cast<T const *>(data());
  }

  template <typename T>
  friend class SpecialisedQueryVariant;

private:
  AbstractQueryVariant(int32_t type, Token token, std::type_index type_index)
    : type_{type}
    , token_{std::move(token)}
    , type_index_{type_index}
  {}

  int32_t         type_;
  Token           token_;
  std::type_index type_index_;
};

template <typename T>
class SpecialisedQueryVariant final : public AbstractQueryVariant
{
public:
  using Token        = fetch::byte_array::Token;
  using QueryVariant = std::shared_ptr<AbstractQueryVariant>;

  static QueryVariant New(T value, int32_t type = 0, Token token = static_cast<Token>(""))
  {
    return QueryVariant(new SpecialisedQueryVariant<T>(std::move(value), type, std::move(token),
                                                       std::type_index(typeid(T))));
  }

  void const *data() const override
  {
    return reinterpret_cast<void const *>(&value_);
  }

private:
  SpecialisedQueryVariant(T value, int32_t type, Token token, std::type_index type_index)
    : AbstractQueryVariant(type, std::move(token), type_index)
    , value_{std::move(value)}
  {}

  T value_;
};

using QueryVariant = std::shared_ptr<AbstractQueryVariant>;

template <typename T>
QueryVariant NewQueryVariant(
    T val, int type = 0,
    AbstractQueryVariant::Token token = static_cast<AbstractQueryVariant::Token>(""))
{
  return SpecialisedQueryVariant<T>::New(std::move(val), type, std::move(token));
}

}  // namespace semanticsearch
}  // namespace fetch
