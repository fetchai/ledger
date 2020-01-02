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

#include "math/matrix_operations.hpp"
#include "ml/ops/add.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace ops {

/**
 * method for updating axes in case of broadcast Add
 * @tparam TensorType
 * @param inputs
 */
template <typename TensorType>
void Add<TensorType>::UpdateAxes(VecTensorType const &inputs)
{
  bool axes_changed = false;

  // Check if axes were changed
  SizeType cnt = 0;
  for (SizeType i{0}; i < inputs.at(0)->shape().size(); i++)
  {
    if (inputs.at(0)->shape().at(i) != inputs.at(1)->shape().at(i))
    {
      if (cnt >= axes_.size() || axes_.at(cnt) != i)
      {
        axes_changed = true;
        break;
      }
      cnt++;
    }
  }

  if (axes_.empty())
  {
    axes_changed = true;
  }

  // Update axes if necessary
  if (axes_changed)
  {
    axes_.clear();
    // Get axes
    for (SizeType i{0}; i < inputs.at(0)->shape().size(); i++)
    {
      if (inputs.at(0)->shape().at(i) != inputs.at(1)->shape().at(i))
      {
        axes_.emplace_back(i);
      }
    }
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Add<math::Tensor<float>>;
template class Add<math::Tensor<double>>;
template class Add<math::Tensor<fixed_point::fp32_t>>;
template class Add<math::Tensor<fixed_point::fp64_t>>;
template class Add<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ops
}  // namespace ml
}  // namespace fetch
