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

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Convolution : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Convolution()          = default;
  virtual ~Convolution() = default;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    // Input should be a 3D tensor [C x H x W]
    assert(inputs[0]->shape().size() == 3);
    // Weights should be a 4D tensor [oC x iC x H x W]
    assert(inputs[1]->shape().size() == 4);

    std::vector<typename ArrayType::SizeType> outputShape;
    outputShape.push_back(inputs[1]->shape()[0]);
    outputShape.push_back(inputs[0]->shape()[1] - inputs[1]->shape()[2] + 1);
    outputShape.push_back(inputs[0]->shape()[2] - inputs[1]->shape()[3] + 1);
    if (!this->output_ || this->output_->shape() != outputShape)
      {
	this->output_ = std::make_shared<ArrayType>(outputShape);
      }
    
    for (uint64_t i(0) ; i < outputShape[0] ; ++i) // Iterate over output channels
      {
	for (uint64_t j(0) ; j < outputShape[1] ; ++j) // Iterate over output height
	  {
	    for (uint64_t k(0) ; k < outputShape[2] ; ++k) // Iterate over output width
	      {
		typename ArrayType::Type sum(0);
		for (uint64_t ki(0) ; ki < inputs[1]->shape()[1] ; ki++) // Iterate over Input channel
		  {
		    for (uint64_t kj(0) ; kj < inputs[1]->shape()[2] ; kj++) // Iterate over kernel height
		      {
			for (uint64_t kk(0) ; kk < inputs[1]->shape()[3] ; kk++) // Iterate over kernel width
			  {
			    std::vector<typename ArrayType::SizeType> kernelIdx({i, ki, kj, kk});
			    std::vector<typename ArrayType::SizeType> inputIdx(3);
			    inputIdx[0] = ki;
			    inputIdx[1] = j + kj;
			    inputIdx[2] = k + kk;
			    typename ArrayType::Type i = inputs[0]->At(inputIdx);
			    typename ArrayType::Type w = inputs[1]->At(kernelIdx);
			    sum += i * w;
			  }
		      }
		  }
		this->output_->Set(std::vector<typename ArrayType::SizeType>({i, j, k}), sum);
	      }
	  }
      }
    
    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    return {errorSignal};
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
