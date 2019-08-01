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

#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/ops.hpp"

#include "ml/ops/embeddings.hpp"
#include "ml/ops/weights.hpp"

//#include "core/assert.hpp"
//#include "math/tensor.hpp"
//#include "ml/saveparams/saveable_params.hpp"

//#include <functional>
//#include <memory>
//#include <vector>

namespace fetch {
namespace ml {
namespace ops {

class OpInterface
{
public:
  //  static std::string Descriptor(OpType const & operation_type);
  //
  //  template <typename T>
  //  static Trainable<T> OpToTrainable(std::shared_ptr<Ops<T>> op_ptr, OpType const &
  //  operation_type)
  //  {
  //    switch (operation_type)
  //    {
  //      case ops::Weights<T>::OpCode():
  //      {
  //        auto weight_ptr = std::dynamic_pointer_cast<Weights<T>>(op_ptr);
  //        return std::dynamic_pointer_cast<Trainable<T>>(weight_ptr);
  //      }
  //      default:
  //        throw std::runtime_error("unknown node type");
  //    }
  //  }

  template <typename T, typename... Params>
  static void Build(OpType const &operation_type, std::shared_ptr<ops::Ops<T>> op_ptr,
                    Params... params);
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
