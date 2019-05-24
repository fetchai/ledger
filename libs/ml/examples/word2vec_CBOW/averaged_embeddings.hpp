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
class AveragedEmbeddings : public fetch::ml::ops::Weights<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;
  static constexpr char const *DESCRIPTOR = "Average Embeddings";

  AveragedEmbeddings(SizeType dimensions, SizeType dataPoints)
  {
    ArrayType weights = ArrayType({dimensions, dataPoints});
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, dimensions, dataPoints);
    this->SetData(weights);
  }

  AveragedEmbeddings(ArrayType &weights)
  {
    this->SetData(weights);
  }

  virtual ~AveragedEmbeddings() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    assert(this->output_);
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape().size() == 2);
    assert(output.shape() == this->ComputeOutputShape(inputs));


    std::cout << DESCRIPTOR << ": " << __func__ << std::endl;
    for(auto &i : inputs)
    {
      std::cout << " --> " << i.get().shape()[0] << " x " << i.get().shape()[1] << std::endl;
      for(auto &x: i.get())
      {
        std::cout << x << ", ";
      }
      std::cout << std::endl;
    }
    std::cout << " --< " << output.shape()[0] << " x " << output.shape()[1] << std::endl;
    std::cout << std::endl;


    uint64_t valid_samples(0);
    // Taking a slice of the output, this as the effect of turning a [1xDIM] matrix into a [DIM]
    // vector (could have used squeeze) This is done for performance reasons as iterating over a
    // vector is much faster than iterating over a matrix

    auto output_slice = output.View(0);
    bool                          clear        = true;
    for (DataType const &i : inputs.front().get())
    {
      if (i >= 0)
      {
        if (clear)
        {
          Assign(output_slice, this->output_->View(typename ArrayType::SizeType(i)));
          clear = false;
        }
        else
        {
          auto slice = this->output_->View(typename ArrayType::SizeType(i));
          PolyfillInlineAdd(output_slice, slice);
        }
        valid_samples++;
      }
    }

    DataType div = static_cast<DataType>(valid_samples);
    std::cout << "Division: " << div << std::endl;
    
    for(auto &val: output_slice)
    {
      val /= div;
    }
    
    // TODO: InlineDivide(output_slice, );
    
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
      ArrayType const &                                           error_signal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape().size() == 2);

    std::cout << DESCRIPTOR << ": " << __func__ << std::endl;
    for(auto &i : inputs)
    {
      std::cout << " --> " << i.get().shape()[0] << " x " << i.get().shape()[1] << std::endl;
    }
    std::cout << " --: " << error_signal.shape()[0] << " x " << error_signal.shape()[1] << std::endl;
    std::cout << std::endl;


    
    // Taking a slice of the output, this as the effect of turning a [1xDIM] matrix into a [DIM]
    // vector (could have used Squeeze) This is done for performance reasons as iterating over a
    // vector is much faster than iterating over a matrix
    auto error_signal_slice = error_signal.View(0);
    
    for (DataType const &i : inputs.front().get())
    {
      if (i >= 0)
      {
        updated_rows_.insert(typename ArrayType::SizeType(double(i)));
        auto slice1 =this->gradient_accumulation_->View(typename ArrayType::SizeType(i));

        PolyfillInlineAdd(slice1, error_signal_slice);
      }
    }
    
    std::cout << "ERROR: ";
    for(std::size_t j=0; j< error_signal.width(); ++j)
    {
      for(std::size_t i=0; i< error_signal.height(); ++i)
      {        
        std::cout << error_signal(i, j) << ", ";
      }
      std::cout << std::endl << std::endl;
    }


    return {error_signal};
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
      std::vector<std::reference_wrapper<ArrayType const>> const & /*inputs*/) const
  {
    std::vector<SizeType> outputShape = this->output_->shape();
    outputShape[1]                                          = 1;
    return outputShape;
  }

private:
  std::set<typename ArrayType::SizeType> updated_rows_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
