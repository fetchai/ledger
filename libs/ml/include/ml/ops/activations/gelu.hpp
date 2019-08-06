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

#include "core/assert.hpp"
#include "math/activation_functions/gelu.hpp"
#include "math/trigonometry.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
	namespace ml {
		namespace ops {
			
template <class T>
class Gelu : public fetch::ml::Ops<T>
{
public:
	using ArrayType     = T;
	using DataType      = typename ArrayType::Type;
	using SizeType      = typename ArrayType::SizeType;
	using VecTensorType = typename Ops<T>::VecTensorType;
	
	Gelu()           = default;
	~Gelu() override = default;
	
	// f(x)=max(0,x);
	void Forward(VecTensorType const &inputs, ArrayType &output) override
	{
		assert(inputs.size() == 1);
		assert(output.shape() == this->ComputeOutputShape(inputs));
		fetch::math::Gelu(*(inputs.front()), output);
	}
	
	/**
	 * Gradients for backprop with Gelu are as follows:
	 * a = 0.797885, b = 0.035677
	 * 0.5 * (1 + tanh(ax + bx^3) + x * sech^2(ax + bx^3)(a + 3bx^2))
	 * @param inputs
	 * @param error_signal
	 * @return
	 */
	std::vector<ArrayType> Backward(VecTensorType const &inputs,
	                                ArrayType const &    error_signal) override
	{
		assert(inputs.size() == 1);
		assert(inputs.at(0)->shape() == error_signal.shape());
		
		ArrayType const &input = *(inputs.front());
		ArrayType intermediate1({input.shape()}), intermediate2({input.shape()}), intermediate3({input.shape()});
		
		DataType one{1}, two{2}, half{static_cast<DataType>(0.5)}, three{3}, a{static_cast<DataType>(0.797885)}, b{static_cast<DataType>(0.035677)};
		
		// get ax + bx^3
		fetch::math::Multiply(input, a, intermediate1);
		fetch::math::Pow(input, three, intermediate2);
		fetch::math::Multiply(intermediate2, b, intermediate2);
		fetch::math::Add(intermediate1, intermediate2, intermediate1);
		
		// get tanh() term
		fetch::math::TanH(intermediate1, intermediate2);
		
		// get x * sech^2() term
		fetch::math::CosH(intermediate1, intermediate3);
		fetch::math::Pow(intermediate3, two, intermediate3);
		fetch::math::Divide(one, intermediate3, intermediate3);
		fetch::math::Multiply(input, intermediate3, intermediate3);
		
		// get a + 3bx^2 term
		fetch::math::Pow(input, two, intermediate1);
		fetch::math::Multiply(intermediate1, b, intermediate1);
		fetch::math::Multiply(intermediate1, three, intermediate1);
		fetch::math::Add(intermediate1, a);
		
		// 1 + tanh(ax + bx^3) + x * sech^2(ax + bx^3)(a + 3bx^2)
		fetch::math::Multiply(intermediate1, intermediate3, intermediate1);
		fetch::math::Add(intermediate1, intermediate2, intermediate1);
		fetch::math::Add(intermediate1, one, intermediate1);
		
		// final value
		fetch::math::Multiply(intermediate1, half, intermediate1);
		
		return intermediate1;
	}
	
	std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
	{
		return inputs.front()->shape();
	}
	
	static constexpr char const *DESCRIPTOR = "Gelu";
};
			
		}  // namespace ops
	}  // namespace ml
}  // namespace fetch
