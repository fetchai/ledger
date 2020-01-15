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
/*
#include "ml/utilities/min_max_scaler.hpp"

namespace fetch {
namespace ml {
namespace utilities {

/
 * Calculate the min, max, and range for reference data
 * @tparam TensorType
 * @param reference_tensor

template <typename TensorType>
void MinMaxScaler<TensorType>::SetScale(TensorType const &reference_tensor)
{

	  SizeType batch_dim = reference_tensor.shape().size() - 1;

	  // first loop computes min and max
	  for (std::size_t i = 0; i < reference_tensor.shape(batch_dim); ++i)
	  {
	    auto ref_it = reference_tensor.View(i).cbegin();
	    while (ref_it.is_valid())
	    {
	      if (x_min_ > *ref_it)
	      {
	        x_min_ = *ref_it;
	      }

	      if (x_max_ < *ref_it)
	      {
	        x_max_ = *ref_it;
	      }
	      ++ref_it;
	    }
	  }

	  x_range_ = x_max_ - x_min_;

}

// template class MinMaxScaler<math::Tensor<int8_t>>;
 // template class MinMaxScaler<math::Tensor<int16_t>>;
 //template class MinMaxScaler<math::Tensor<int32_t>>;
 //template class MinMaxScaler<math::Tensor<int64_t>>;
 template class MinMaxScaler<math::Tensor<float>>;
 template class MinMaxScaler<math::Tensor<double>>;
 template class MinMaxScaler<math::Tensor<fixed_point::fp32_t>>;
 template class MinMaxScaler<math::Tensor<fixed_point::fp64_t>>;
 template class MinMaxScaler<math::Tensor<fixed_point::fp128_t>>;

}
}
}

*/


