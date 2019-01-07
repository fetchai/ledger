#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2019 Fetch.AI Limited
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

#include "math/free_functions/free_functions.hpp"
#include "ml/ops/derivatives/derivatives.hpp"
#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

  template <class T>
  class MatrixMultiply : public fetch::ml::Ops<T>
  {
  public:
    using ArrayType         = T;
    using ArrayPtrType      = std::shared_ptr<ArrayType>;

    MatrixMultiply() = default;

    virtual ArrayPtrType forward(std::vector<ArrayPtrType> const & inputs)
    {
      assert(inputs.size() == 2);
      assert(inputs[0]->shape().size() == 2);
      assert(inputs[1]->shape().size() == 2);
      assert(inputs[0]->shape()[1] == inputs[1]->shape()[0]);

      // std::cout << "[" << inputs[0]->shape()[0] << " x " << inputs[0]->shape()[1] <<
      // 	"] * [" << inputs[1]->shape()[0] << " x " << inputs[1]->shape()[1] << "]" <<
      // 	" => [" << inputs[0]->shape()[0] << " x " <<  inputs[1]->shape()[1] << "]"<< std::endl;	
      
      std::vector<size_t> outputShape({inputs[0]->shape()[0], inputs[1]->shape()[1]});
      if (!this->output_ || this->output_->shape() != outputShape)
        this->output_ = std::make_shared<ArrayType>(outputShape);

      fetch::math::Dot(*(inputs[0]), *(inputs[1]), *(this->output_));
      
      return this->output_;
    }
    
    virtual std::vector<ArrayPtrType> backward(std::vector<ArrayPtrType> const & inputs, ArrayPtrType errorSignal)
    {
      assert(inputs.size() == 2);

      ArrayPtrType errorSignal1 = std::make_shared<ArrayType>(inputs[0]->shape());
      ArrayPtrType errorSignal2 = std::make_shared<ArrayType>(inputs[1]->shape());
      
      fetch::math::DotTranspose(*errorSignal, *inputs[1], *errorSignal1);
      fetch::math::TransposeDot(*inputs[0], *errorSignal, *errorSignal2);

      return {errorSignal1, errorSignal2};
    }
    
  private:
    // Relu is done in a strange way, comparing input against an array of zeroes
    // using a parrallel Maximum function 
    ArrayPtrType zeroes_;
  };

}  // namespace ops
}  // namespace ml
}  // namespace fetch
