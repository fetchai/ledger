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

#include "ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class PlaceHolder : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using SizeType     = typename ArrayType::SizeType;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  PlaceHolder() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    (void)output;
    (void)inputs; 
    assert(inputs.empty());
    assert(this->output_);
    
    std::cout << DESCRIPTOR << ": " << __func__ << std::endl;
    for(auto &i : inputs)
    {
      std::cout << " --> " << i.get().shape()[0] << " x " << i.get().shape()[1] << std::endl;
    }
    std::cout << " --< " << output.shape()[0] << " x " << output.shape()[1] << std::endl;
    std::cout << std::endl;

    std::cout << "OUTPUT: ";
    for(auto &o: *(this->output_))
    {
      std::cout << o << ", ";
    }
    std::cout << std::endl;
//    exit(-1);
    
    return *(this->output_);
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    (void)inputs;     
    assert(inputs.empty());
    std::cout << "ERROR: ";
    for(std::size_t j=0; j< errorSignal.width(); ++j)
    {
      for(std::size_t i=0; i< errorSignal.height(); ++i)
      {        
        std::cout << errorSignal(i, j) << ", ";
      }
      std::cout << std::endl << std::endl;
    }

    return {errorSignal};
  }

  virtual bool SetData(ArrayType const &data)
  {
    std::vector<SizeType> old_shape;
    if (this->output_)
    {
      old_shape = this->output_->shape();
    }
    this->output_ = std::make_shared<ArrayType>(data);
    return old_shape != this->output_->shape();
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    (void)inputs;
    return this->output_->shape();
  }

  static constexpr char const *DESCRIPTOR = "PlaceHolder";

protected:
  ArrayPtrType output_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
