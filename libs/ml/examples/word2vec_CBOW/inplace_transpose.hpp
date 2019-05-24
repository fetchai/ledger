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
class InplaceTranspose : public fetch::ml::BatchOps<T>
{
public:
  using ArrayType = T;
  using SizeType  = typename ArrayType::SizeType;

  InplaceTranspose()          = default;
  virtual ~InplaceTranspose() = default;

  ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                    ArrayType &                                                 output)
  {
    (void)output;  // Transposing input inplace
    assert(inputs.size() == 1);
    
    std::cout << DESCRIPTOR << ": " << __func__ << std::endl;
    for(auto &i : inputs)
    {
      std::cout << " --> " << i.get().shape()[0] << " x " << i.get().shape()[1] << std::endl;
    }
    std::cout << " --< " << output.shape()[0] << " x " << output.shape()[1] << std::endl;
    std::cout << std::endl;

    output = inputs.front().get().Transpose();

    std::cout << "OUTPUT: ";
    for(auto &o: output)
    {
      std::cout << o << ", ";
    }
    std::cout << std::endl;    
    
    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 1);
    std::cout << DESCRIPTOR << ": " << __func__ << std::endl;
    for(auto &i : inputs)
    {
      std::cout << " --> " << i.get().shape()[0] << " x " << i.get().shape()[1] << std::endl;
    }
    std::cout << " --: " << errorSignal.shape()[0] << " x " << errorSignal.shape()[1] << std::endl;
    std::cout << std::endl;

    (void)inputs;   
    auto output = errorSignal.Transpose();
    std::cout << "ERROR: ";
    for(std::size_t j=0; j< output.width(); ++j)
    {
      for(std::size_t i=0; i< output.height(); ++i)
      {        
        std::cout << output(i, j) << ", ";
      }
      std::cout << std::endl << std::endl;
    }

    return {output};
  }

  std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    return {inputs.front().get().shape()[1], inputs.front().get().shape()[0]};
  }

  static constexpr char const *DESCRIPTOR = "Transpose";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
