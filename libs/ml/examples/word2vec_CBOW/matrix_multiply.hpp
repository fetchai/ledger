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
#include "ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class MatrixMultiply : public fetch::ml::BatchOps<T>
{
public:
  using ArrayType = T;
  using SizeType  = typename ArrayType::SizeType;

  MatrixMultiply()  = default;
  ~MatrixMultiply() = default;

  ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                    ArrayType &                                                 output)
  {
    (void)output;
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape().size() == 2);
    assert(inputs.at(1).get().shape().size() == 2);
    assert(output.shape() == ComputeOutputShape(inputs));

    std::cout << DESCRIPTOR << ": " << __func__ << std::endl;
    for(auto &i : inputs)
    {
      std::cout << " --> " << i.get().shape()[0] << " x " << i.get().shape()[1] << std::endl;
    }
    std::cout << " --< " << output.shape()[0] << " x " << output.shape()[1] << std::endl;
    std::cout << std::endl;

    fetch::math::TransposeDot(inputs[1].get(), inputs[0].get(),  output);

    std::cout << "OUTPUT: ";
    for(auto &o: output)
    {
      std::cout << o << ", ";
    }
    std::cout << std::endl;    

    return output;
  }

  std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 2);

    std::cout << DESCRIPTOR << ": " << __func__ << std::endl;
    for(auto &i : inputs)
    {
      std::cout << " --> " << i.get().shape()[0] << " x " << i.get().shape()[1] << std::endl;
    }
    std::cout << " --: " << errorSignal.shape()[0] << " x " << errorSignal.shape()[1] << std::endl;
    std::cout << std::endl;


    ArrayType errorSignal1(inputs.at(0).get().shape());
    ArrayType errorSignal2(inputs.at(1).get().shape());

    fetch::math::DotTranspose(errorSignal, inputs.at(1).get(), errorSignal1);
    fetch::math::TransposeDot(inputs.at(0).get(), errorSignal, errorSignal2);

    std::cout << "ERROR 1: ";
    for(std::size_t j=0; j< errorSignal1.width(); ++j)
    {
      for(std::size_t i=0; i< errorSignal1.height(); ++i)
      {        
        std::cout << errorSignal1(i, j) << ", ";
      }
      std::cout << std::endl << std::endl;
    }

    std::cout << "ERROR 2: ";
    for(std::size_t j=0; j< errorSignal2.width(); ++j)
    {
      for(std::size_t i=0; i< errorSignal2.height(); ++i)
      {        
        std::cout << errorSignal2(i, j) << ", ";
      }
      std::cout << std::endl << std::endl;
    }



    return {errorSignal1, errorSignal2};
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    return {inputs.at(1).get().shape()[1], inputs.at(0).get().shape()[1]};
  }

  static constexpr char const *DESCRIPTOR = "MatrixMultiply";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
