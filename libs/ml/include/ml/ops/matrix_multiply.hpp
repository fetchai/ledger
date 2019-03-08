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

#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MatrixMultiply : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  MatrixMultiply()          = default;
  virtual ~MatrixMultiply() = default;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 2);
    assert(inputs[0]->shape().size() == 2 || inputs[0]->shape().size() == 3);
    assert(inputs[1]->shape().size() == 2 || inputs[1]->shape().size() == 3);

    // Indicate if we're running in batch mode
    bool unsqueeze = (inputs[0]->shape().size() == 2);
    if (unsqueeze)
      {
	inputs[0]->Unsqueeze();
	inputs[1]->Unsqueeze();
      }

    std::vector<std::uint64_t> outputShape({inputs[0]->shape()[0], inputs[0]->shape()[1], inputs[1]->shape()[2]});
    if (!this->output_ || this->output_->shape() != outputShape)
    {
      this->output_ = std::make_shared<ArrayType>(outputShape);
    }

    for (uint64_t b(0) ; b < inputs[0]->shape()[0] ; ++b)
      {
	ArrayType inputBatch1 = inputs[0]->Slice(b);
	ArrayType inputBatch2 = inputs[1]->Slice(b);
	ArrayType outputBatch = this->output_->Slice(b);
	assert(inputBatch1shape()[1] == inputBatch2shape()[0]);
	for (std::uint64_t i(0); i < inputBatch1.shape()[0] ; ++i)
	  {
	    for (std::uint64_t j(0); j < inputBatch2.shape()[1] ; ++j)
	      {
		outputBatch.At(std::vector<std::uint64_t>({i, j})) =
		  inputBatch1.At(std::vector<std::uint64_t>({i, 0})) *
		  inputBatch2.At(std::vector<std::uint64_t>({0, j}));
		for (std::uint64_t k(1); k < inputBatch1.shape()[1] ; ++k)
		  {
		    outputBatch.At(std::vector<std::uint64_t>({i, j})) +=
		      inputBatch1.At(std::vector<std::uint64_t>({i, k})) *
		      inputBatch2.At(std::vector<std::uint64_t>({k, j}));
		  }
	      }
	  }
      }

    // Go back to input data format
    if (unsqueeze)
      {
	inputs[0]->Squeeze();
	inputs[1]->Squeeze();
	this->output_->Squeeze();
      }
    
    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    assert(inputs.size() == 2);

    // Indicate if we're running in batch mode
    bool unsqueeze = (inputs[0]->shape().size() == 2);
    if (unsqueeze)
      {
	inputs[0]->Unsqueeze();
	inputs[1]->Unsqueeze();
	errorSignal->Unsqueeze();
      }
    

    ArrayPtrType errorSignal1 = std::make_shared<ArrayType>(inputs[0]->shape());
    ArrayPtrType errorSignal2 = std::make_shared<ArrayType>(inputs[1]->shape());

    for (uint64_t b(0) ; b < inputs[0]->shape().front() ; ++b) // Looping over batch dimension
      {
	ArrayType input1Batch = inputs[0]->Slice(b);
	ArrayType input2Batch = inputs[1]->Slice(b);
	ArrayType errorSignalBatch = errorSignal->Slice(b);
	ArrayType errorSignal1Batch = errorSignal1->Slice(b);
	ArrayType errorSignal2Batch = errorSignal2->Slice(b);
	
	fetch::math::DotTranspose(errorSignalBatch, input2Batch, errorSignal1Batch);
	fetch::math::TransposeDot(input1Batch, errorSignalBatch, errorSignal2Batch);
      }

    if (unsqueeze)
      {
	inputs[0]->Squeeze();
	inputs[1]->Squeeze();
	errorSignal->Squeeze();
	errorSignal1->Squeeze();
	errorSignal2->Squeeze();
      }

    return {errorSignal1, errorSignal2};
  }

  static constexpr char const *DESCRIPTOR = "MatrixMultiply";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
