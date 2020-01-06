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
  using Vocabulary = std::shared_ptr<VocabularyInstance>;
  template <typename T>
  using Reducer = std::function<SemanticPosition(T const &)>;
  template <typename T>
  using Validator = std::function<bool(T const &)>;

  SemanticReducer()                             = default;
  SemanticReducer(SemanticReducer const &other) = default;
  SemanticReducer &operator=(SemanticReducer const &other) = default;

  template <typename T>
  void SetReducer(int32_t rank, Reducer<T> reducer)
  {
    rank_    = rank;
    reducer_ = [reducer](void *data) { return reducer(*reinterpret_cast<T *>(data)); };
    if (reducer_ == nullptr)
    {
      rank_ = 0;
    }
  }

  template <typename T>
  void SetValidator(Validator<T> validator)
  {
    constraints_validation_ = [validator](void *data) {
      return validator(*reinterpret_cast<T *>(data));
    };
  }

  bool             Validate(void *data);
  SemanticPosition Reduce(void *data);
  int32_t          rank() const;

private:
  using InternalReducer   = std::function<SemanticPosition(void *)>;
  using InternalValidator = std::function<bool(void *)>;

  int32_t           rank_{0};
  InternalReducer   reducer_{nullptr};
  InternalValidator constraints_validation_{nullptr};
};

}  // namespace semanticsearch
}  // namespace fetch
