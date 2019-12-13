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

#include "semanticsearch/index/base_types.hpp"
#include "semanticsearch/schema/vocabulary_instance.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace fetch {
namespace semanticsearch {

class SemanticReducer
{
public:
  template <typename T>
  using Reducer = std::function<SemanticPosition(T const &)>;
  template <typename T>
  using Validator = std::function<bool(T const &, std::string &error)>;

  /// Constructors
  /// @{
  SemanticReducer(std::string const &uid);
  SemanticReducer(SemanticReducer const &other) = default;
  SemanticReducer &operator=(SemanticReducer const &other) = default;
  /// @}

  /// Setup
  /// @{
  template <typename T>
  void SetReducer(int32_t rank, Reducer<T> reducer);
  template <typename T>
  void SetValidator(Validator<T> validator);
  /// @}

  /// Comparison
  /// @{
  bool operator==(SemanticReducer const &other) const;
  bool operator<=(SemanticReducer const &other) const;
  bool operator<(SemanticReducer const &other) const;
  bool operator>=(SemanticReducer const &other) const;
  bool operator>(SemanticReducer const &other) const;
  /// @}

  /// Used for execution
  /// @{
  template <typename T>
  bool Validate(T const &data, std::string &error) const;
  template <typename T>
  SemanticPosition Reduce(T const &data) const;
  /// @}

  /// Properties
  /// @{
  int32_t rank() const;
  /// @}
private:
  using InternalReducer   = std::function<SemanticPosition(void const *)>;
  using InternalValidator = std::function<bool(void const *, std::string &)>;

  /// Internals for execution
  /// @{
  bool             Validate(void const *data, std::string &error) const;
  SemanticPosition Reduce(void const *data) const;
  /// @}

  /// Data members
  /// @{
  std::string       unique_identifier_{};  ///< Identifier that makes reducers comparible.
  int32_t           rank_{0};              ///< Rank of the reducer
  InternalReducer   reducer_{nullptr};     ///< Reducer that produces a vector
  InternalValidator constraints_validation_{nullptr};  ///< Constraint validator
  /// @}
};

template <typename T>
bool SemanticReducer::Validate(T const &data, std::string &error) const
{
  return Validate(reinterpret_cast<void const *>(&data), error);
}

template <typename T>
SemanticPosition SemanticReducer::Reduce(T const &data) const
{
  return Reduce(reinterpret_cast<void const *>(&data));
}

template <typename T>
void SemanticReducer::SetReducer(int32_t rank, Reducer<T> reducer)
{
  if (reducer == nullptr)
  {
    rank_    = 0;
    reducer_ = nullptr;
  }
  else
  {
    rank_    = rank;
    reducer_ = [reducer](void const *data) { return reducer(*reinterpret_cast<T const *>(data)); };
  }
}

template <typename T>
void SemanticReducer::SetValidator(Validator<T> validator)
{
  constraints_validation_ = [validator](void const *data, std::string &error) {
    return validator(*reinterpret_cast<T const *>(data), error);
  };
}
}  // namespace semanticsearch
}  // namespace fetch
