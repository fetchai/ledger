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

#include "weights.hpp"
#include "polyfill.hpp"
#include <set>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Embeddings : public fetch::ml::ops::Weights<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;
  static constexpr char const *DESCRIPTOR = "Embeddings";

  Embeddings(SizeType dataPoints, SizeType dimensions)
  {
    ArrayType weights = ArrayType({dataPoints, dimensions});
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dataPoints, dimensions);
    this->SetData(weights);
  }

  Embeddings(ArrayType &weights)
  {
    this->SetData(weights);
  }

  virtual ~Embeddings() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    assert(this->output_);
    assert(inputs.size() == 1);
    assert(output.shape() == ComputeOutputShape(inputs));

    std::cout << DESCRIPTOR << ": " << __func__ << std::endl;
    for(auto &i : inputs)
    {
      std::cout << " --> " << i.get().shape()[0] << " x " << i.get().shape()[1] << std::endl;
    }
    std::cout << " --< " << output.shape()[0] << " x " << output.shape()[1] << std::endl;
    std::cout << std::endl;


    uint64_t j=0;
    for (DataType const &i : inputs.front().get())
    {
      auto slice1 = output.View(j);
      auto slice2 = this->output_->View(SizeType(i));
      Assign(slice1, slice2);
      j++;
    }

    std::cout << "OUTPUT: ";

    for(std::size_t j=0; j< output.width(); ++j)
    {
      for(std::size_t i=0; i< output.height(); ++i)
      {        
        std::cout << output(i, j) << ", ";
      }
      std::cout << std::endl << std::endl;
    }

    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 1);

    uint64_t j(0);
    for (DataType const &i : inputs.front().get())
    {
      updated_rows_.insert(typename ArrayType::SizeType(double(i)));

      auto slice1 = this->gradient_accumulation_->View(typename ArrayType::SizeType(double(i)));
      auto slice2 = errorSignal.View(j);

      PolyfillInlineAdd(slice1, slice2);

      j++;
    }
    return {errorSignal};
  }

  virtual void Step(typename T::Type learningRate)
  {
    for (auto const &r : updated_rows_)
    {
      auto gradientAccumulationSlice = this->gradient_accumulation_->View(r);
      auto outputSlice               = this->output_->View(r);
      auto it1                       = gradientAccumulationSlice.begin();

      auto it2                       = outputSlice.begin();
      while (it1.is_valid())
      {
        *it2 += (*it1 * learningRate);
        *it1 = 0;
        ++it1;
        ++it2;
      }
    }
    updated_rows_.clear();
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    return {this->output_->shape()[0], inputs.front().get().size()};
  }

private:
  std::set<typename ArrayType::SizeType> updated_rows_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
