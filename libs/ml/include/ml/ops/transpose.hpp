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
class Transpose : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Transpose()          = default;
  virtual ~Transpose() = default;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    ASSERT(inputs.size() == 1);
    if (inputs[0]->shape().size() == 2)  // Non batch
    {
      this->output_ = std::make_shared<ArrayType>(inputs[0]->Clone().Transpose());
    }
    else if (inputs[0]->shape().size() == 3)  // Batch
    {
      std::vector<typename ArrayType::SizeType> inputShape = inputs[0]->shape();
      this->output_                                        = std::make_shared<ArrayType>(
          std::vector<typename ArrayType::SizeType>({inputShape[0], inputShape[2], inputShape[1]}));
      for (unsigned int i(0); i < inputShape[0]; ++i)
      {
        this->output_->Slice(i).Copy(inputs[0]->Slice(i).Transpose());
      }
    }
    else
    {
      throw std::runtime_error("Can't transpose this tensor");
    }
    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    ASSERT(inputs.size() == 1);
    if (inputs[0]->shape().size() == 2)  // Non batch
    {
      return {std::make_shared<ArrayType>(errorSignal->Clone().Transpose())};
    }
    else if (inputs[0]->shape().size() == 3)  // Batch
    {
      std::vector<typename ArrayType::SizeType> inputShape = inputs[0]->shape();
      ArrayPtrType                              ret        = std::make_shared<ArrayType>(
          std::vector<typename ArrayType::SizeType>({inputShape[0], inputShape[2], inputShape[1]}));
      for (unsigned int i(0); i < inputShape[0]; ++i)
      {
        ret->Slice(i).Copy(errorSignal->Slice(i).Transpose());
      }
      return {ret};
    }
    else
    {
      throw std::runtime_error("Can't transpose this tensor");
    }
    return {nullptr};
  }

  static constexpr char const *DESCRIPTOR = "Transpose";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
