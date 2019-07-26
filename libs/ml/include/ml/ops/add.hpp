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

#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Add : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  Add()           = default;
  ~Add() override = default;
	
  // for inputs to the add layer, if broadcasting is required, make sure the first input is the one with the complete shape
  
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 2);
	  assert(inputs.at(0)->shape().size() <= 3); // we do not support input of more than 3D (including batch dims)
    assert(output.shape() == inputs.at(0)->shape());
    // we allow addition forward as long as the second input has shape broadcastable on first input
    fetch::math::Add((*inputs.at(0)), (*inputs.at(1)), output);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->shape().size() <= 3); // we do not support input of more than 3D (including batch dims)
    assert(inputs.at(0)->shape().size() == inputs.at(1)->shape().size()); // check if addition is broadcastable
    assert(inputs.at(0)->shape() == error_signal.shape());
	
	  if (inputs.at(0)->shape() == inputs.at(1)->shape())
	  {
		  return {error_signal, error_signal};
	  }else if(inputs.at(1)->size() == 1) {
		  // if second input is a scalar
		  auto second_error_signal = ArrayType(inputs.at(1)->shape().at(0));
		  fetch::math::Sum(error_signal, *second_error_signal.begin());
		  return {error_signal, second_error_signal};
	  }
    else{
		  // since the shape is not compatible, then the second input must have size 1 in batch dims
		  SizeType batch_dimension = inputs.at(0)->shape().size() - 1;
		  assert(inputs.at(1)->shape().at(batch_dimension) == 1);
		  if (inputs.at(1)->shape().size() == 2){
		  	// the bias addition case
			  return {error_signal, fetch::math::ReduceSum(error_signal, batch_dimension)};
		  }else{ // in the case where we have three dims
					// We only support backward broadcast through shape (N, 1, 1)
					assert(inputs.at(1)->shape(1) == 1);
					
					ArrayType error_sum({inputs.at(1)->shape(0), 1});
					for(SizeType batch=0; batch < error_signal.shape(batch_dimension); batch++){
//						error_sum += fetch::math::ReduceSum(inputs.at(0)->View(batch).Copy(), SizeType(1));
						error_sum += fetch::math::ReduceSum(error_signal.View(batch).Copy(), SizeType(1));
					}
					error_sum.Reshape(inputs.at(1)->shape());
			    return {error_signal, error_sum};
		  }
    }
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.at(0)->shape();
  }

  static constexpr char const *DESCRIPTOR = "Add";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
