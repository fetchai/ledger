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

#include "math/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LayerNorm : public Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using DataType      = typename ArrayType::Type;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  explicit LayerNorm(std::vector<SizeType> const &data_shape, DataType epsilon = fetch::math::function_tolerance<DataType>())
    : data_shape_(data_shape),
      epsilon_(epsilon)
  {}
  ~LayerNorm() override = default;
	
	std::vector<SizeType> ComputeOutputShape(
	 VecTensorType const &inputs) const override
	{
		return inputs.at(0)->shape();
	}
	
	std::vector<ArrayType> normalize_2D_input(ArrayType const &input, DataType epsilon, SizeType axis){
		assert(axis == 0 || axis == 1);
		assert(input.shape().size() == 2);
		
		// recenter input
		auto s0 = input.ToString();
		ArrayType input_mean = fetch::math::ReduceMean(input, axis);
		auto s1 = input_mean.ToString();
		ArrayType zero_centered_input = fetch::math::Subtract(input, input_mean);
		auto s2 = zero_centered_input.ToString();
		
		// get variance of input
		ArrayType squared_zero_centered_input = fetch::math::Square(zero_centered_input);
		auto s3 = squared_zero_centered_input.ToString();
		ArrayType input_variance = fetch::math::ReduceMean(squared_zero_centered_input, axis);
		auto s4 = input_variance.ToString();
		input_variance = fetch::math::Add(input_variance, epsilon);
		
		// normalize input
		ArrayType sqrt_variance = fetch::math::Sqrt(input_variance);
		auto s5 = sqrt_variance.ToString();
		ArrayType inv_sqrt_variance = fetch::math::Divide(static_cast<DataType>(1), input_variance);
		ArrayType normalized_input = fetch::math::Divide(zero_centered_input, sqrt_variance);
		auto s6 = normalized_input.ToString();
		
		return {normalized_input, zero_centered_input, inv_sqrt_variance, sqrt_variance};
	}
	
	std::vector<ArrayType> normalize_3D_input(ArrayType const &input, DataType epsilon, SizeType axis){
		assert(axis == 0 || axis == 1);
		assert(input.shape().size() == 3);
		
		ArrayType normalized_input(input.shape()), zero_centered_input(input.shape()), inv_sqrt_variance({1, input.shape(1), input.shape(2)}), sqrt_variance({1, input.shape(1), input.shape(2)});
		
		for(SizeType batch = 0; batch < input.shape(2); batch++){
			auto norm_output = normalize_2D_input(input.View(batch).Copy(), epsilon, axis);
			normalized_input.View(batch).Assign(norm_output.at(0));
			zero_centered_input.View(batch).Assign(norm_output.at(1));
			inv_sqrt_variance.View(batch).Assign(norm_output.at(2));
			sqrt_variance.View(batch).Assign(norm_output.at(3));
		}
		return {normalized_input, zero_centered_input, inv_sqrt_variance, sqrt_variance};
	}

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
		// cache this inputs
		prev_inputs_ = {};
		for(auto i: inputs){
			prev_inputs_.emplace_back(*i);
		}
		
		// the layernorm can only be applied on the first axis
		assert(inputs.size() == 1);
		ArrayType input = *(inputs.front());
		
		// we only support input with at most 3 dims
		assert(input.shape().size() <= 3);
		assert(output.shape() == input.shape());
	
	  std::vector<ArrayType> normalization_output;
		if(input.shape().size() == 2){
			normalization_output = normalize_2D_input(input, epsilon_, static_cast<SizeType>(0));
		}else{
			assert(input.shape().size() == 3);
			normalization_output = normalize_3D_input(input, epsilon_, static_cast<SizeType>(0));
		}
	
	  output = normalization_output.at(0);
		
		// cache data for backward part
		cached_data_ = normalization_output;
		cached_output_ = output;
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &error_signal) override
  {
		// make sure we have run forward for this inputs
		assert(inputs.size() == prev_inputs_.size());
		bool is_cached = true;
		for(size_t i = 0; i < prev_inputs_.size(); i++){
			if(*inputs.at(i) != prev_inputs_.at(i)){
				is_cached = false;
				break;
			}
		}
		if(!is_cached){
			cached_output_.Reshape(inputs.front()->shape());
			Forward(inputs, cached_output_);
		}
		
		return {error_signal};
  }

  static constexpr char const *DESCRIPTOR = "Convolution1D";

private:
	std::vector<SizeType> data_shape_;
	DataType epsilon_;
	
	std::vector<ArrayType> prev_inputs_;
	std::vector<ArrayType> cached_data_;
	ArrayType              cached_output_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch