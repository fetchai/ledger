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

#include "ml/exceptions/exceptions.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class StridedSlice : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using SizeVector    = fetch::math::SizeVector;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpStridedSliceSaveableParams<T>;
  using MyType        = StridedSlice<TensorType>;

  explicit StridedSlice(SizeVector const &begins, SizeVector const &ends,
                        SizeVector const &strides = {})
    : begins_(begins)
    , ends_(ends)
    , strides_(strides)

  {
    assert(begins.size() == ends.size());
    if (strides.empty())
    {
      strides_ = begins;
      for (SizeType i{0}; i < strides.size(); i++)
      {
        strides_.at(i) = 1;
      }
    }
  }

  explicit StridedSlice(SPType const &sp)
    : Ops<T>(sp)
  {
    begins_  = sp.begins;
    ends_    = sp.ends;
    strides_ = sp.strides;
  }

  ~StridedSlice() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp = std::make_shared<SPType>();

    sp->begins  = begins_;
    sp->ends    = ends_;
    sp->strides = strides_;

    return sp;
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    return std::make_shared<MyType>(*this);  // calls default copy constructor of MyType;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  SizeVector ComputeOutputShape(VecTensorType const &inputs) const override
  {

    SizeVector output_shape = inputs.front()->shape();

    // Calculate number of stride size steps from specified begin to specified end for each
    // dimension
    for (SizeType i{0}; i < begins_.size(); i++)
    {
      assert(strides_.at(i) != 0);
      assert(begins_.at(i) <= ends_.at(i));
      output_shape.at(i) = ((ends_.at(i) - begins_.at(i)) / strides_.at(i)) + 1;
    }

    return output_shape;
  }

  SizeVector begins_;
  SizeVector ends_;
  SizeVector strides_;

  static constexpr OpType OpCode()
  {
    return OpType::OP_STRIDED_SLICE;
  }

  static constexpr char const *DESCRIPTOR = "StridedSlice";
};

/**
 * Forward pass is done by assigning values in given ranges with stride size step for every
 * dimmension from larger input tensor to smaller output tensor.
 * @tparam T
 * @param inputs
 * @param output
 */
template <class T>
void StridedSlice<T>::Forward(VecTensorType const &inputs, TensorType &output)
{
  assert(inputs.size() == 1);
  assert(output.shape() == this->ComputeOutputShape(inputs));

  switch (inputs.at(0)->shape().size())
  {
    // 1D Tensor
  case 1:
  {
    for (SizeType i{0}; i < output.shape().at(0); i++)
    {
      output.At(i) = inputs.at(0)->At(begins_.at(0) + i * strides_.at(0));
    }
    break;
  }

  // 2D Tensor
  case 2:
  {
    for (SizeType i{0}; i < output.shape().at(0); i++)
    {
      for (SizeType j{0}; j < output.shape().at(1); j++)
      {
        output.At(i, j) = inputs.at(0)->At(begins_.at(0) + i * strides_.at(0),
                                           begins_.at(1) + j * strides_.at(1));
      }
    }
    break;
  }

  // 3D Tensor
  case 3:
  {
    for (SizeType i{0}; i < output.shape().at(0); i++)
    {
      for (SizeType j{0}; j < output.shape().at(1); j++)
      {
        for (SizeType k{0}; k < output.shape().at(2); k++)
        {
          output.At(i, j, k) = inputs.at(0)->At(begins_.at(0) + i * strides_.at(0),
                                                begins_.at(1) + j * strides_.at(1),
                                                begins_.at(2) + k * strides_.at(2));
        }
      }
    }
    break;
  }

  // 4D Tensor
  case 4:
  {
    for (SizeType i{0}; i < output.shape().at(0); i++)
    {
      for (SizeType j{0}; j < output.shape().at(1); j++)
      {
        for (SizeType k{0}; k < output.shape().at(2); k++)
        {
          for (SizeType l{0}; l < output.shape().at(3); l++)
          {
            output.At(i, j, k, l) = inputs.at(0)->At(
                begins_.at(0) + i * strides_.at(0), begins_.at(1) + j * strides_.at(1),
                begins_.at(2) + k * strides_.at(2), begins_.at(3) + l * strides_.at(3));
          }
        }
      }
    }
    break;
  }

  // 5D Tensor
  case 5:
  {
    for (SizeType i{0}; i < output.shape().at(0); i++)
    {
      for (SizeType j{0}; j < output.shape().at(1); j++)
      {
        for (SizeType k{0}; k < output.shape().at(2); k++)
        {
          for (SizeType l{0}; l < output.shape().at(3); l++)
          {
            for (SizeType m{0}; m < output.shape().at(4); m++)
            {

              output.At(i, j, k, l, m) = inputs.at(0)->At(
                  begins_.at(0) + i * strides_.at(0), begins_.at(1) + j * strides_.at(1),
                  begins_.at(2) + k * strides_.at(2), begins_.at(3) + l * strides_.at(3),
                  begins_.at(4) + m * strides_.at(4));
            }
          }
        }
      }
    }
    break;
  }
  default:
  {
    throw exceptions::InvalidMode("Input shape is supported up to 5D only.");
  }
  }
}

/**
 * Backward pass is done by assigning smaller error signal tensor to larger return signal tensor
 * @tparam T
 * @param inputs
 * @param error_signal
 * @return
 */
template <class T>
std::vector<T> StridedSlice<T>::Backward(VecTensorType const &inputs,
                                         TensorType const &   error_signal)
{
  assert(inputs.size() == 1);
  assert(error_signal.shape() == this->ComputeOutputShape(inputs));

  TensorType ret_error_signal_{inputs.at(0)->shape()};

  switch (inputs.at(0)->shape().size())
  {

  // 1D Tensor
  case 1:
  {
    for (SizeType i{0}; i < error_signal.shape().at(0); i++)
    {
      ret_error_signal_.At(begins_.at(0) + i * strides_.at(0)) = error_signal.At(i);
    }
    break;
  }

  // 2D Tensor
  case 2:
  {
    for (SizeType i{0}; i < error_signal.shape().at(0); i++)
    {
      for (SizeType j{0}; j < error_signal.shape().at(1); j++)
      {
        ret_error_signal_.At(begins_.at(0) + i * strides_.at(0),
                             begins_.at(1) + j * strides_.at(1)) = error_signal.At(i, j);
      }
    }
    break;
  }

  // 3D Tensor
  case 3:
  {
    for (SizeType i{0}; i < error_signal.shape().at(0); i++)
    {
      for (SizeType j{0}; j < error_signal.shape().at(1); j++)
      {
        for (SizeType k{0}; k < error_signal.shape().at(2); k++)
        {
          ret_error_signal_.At(begins_.at(0) + i * strides_.at(0),
                               begins_.at(1) + j * strides_.at(1),
                               begins_.at(2) + k * strides_.at(2)) = error_signal.At(i, j, k);
        }
      }
    }
    break;
  }

  // 4D Tensor
  case 4:
  {
    for (SizeType i{0}; i < error_signal.shape().at(0); i++)
    {
      for (SizeType j{0}; j < error_signal.shape().at(1); j++)
      {
        for (SizeType k{0}; k < error_signal.shape().at(2); k++)
        {
          for (SizeType l{0}; l < error_signal.shape().at(3); l++)
          {
            ret_error_signal_.At(begins_.at(0) + i * strides_.at(0),
                                 begins_.at(1) + j * strides_.at(1),
                                 begins_.at(2) + k * strides_.at(2),
                                 begins_.at(3) + l * strides_.at(3)) = error_signal.At(i, j, k, l);
          }
        }
      }
    }
    break;
  }

  // 5D Tensor
  case 5:
  {
    for (SizeType i{0}; i < error_signal.shape().at(0); i++)
    {
      for (SizeType j{0}; j < error_signal.shape().at(1); j++)
      {
        for (SizeType k{0}; k < error_signal.shape().at(2); k++)
        {
          for (SizeType l{0}; l < error_signal.shape().at(3); l++)
          {
            for (SizeType m{0}; m < error_signal.shape().at(4); m++)
            {
              ret_error_signal_.At(
                  begins_.at(0) + i * strides_.at(0), begins_.at(1) + j * strides_.at(1),
                  begins_.at(2) + k * strides_.at(2), begins_.at(3) + l * strides_.at(3),
                  begins_.at(4) + m * strides_.at(4)) = error_signal.At(i, j, k, l, m);
            }
          }
        }
      }
    }
    break;
  }
  default:
  {
    throw exceptions::InvalidMode("Input shape is supported up to 5D only.");
  }
  }

  return {ret_error_signal_};
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
