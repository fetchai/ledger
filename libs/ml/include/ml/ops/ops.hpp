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

#include "core/assert.hpp"
#include "math/tensor.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

/*
 * Abstract Ops interface
 */
template <class T>
class Ops
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = std::vector<std::shared_ptr<TensorType const>>;
  using VecShapesType = std::vector<std::vector<SizeType>>;
  using ChargeAmount  = uint64_t;  // TODO(VH): Move me to an external declaration header.

  virtual ~Ops() = default;

  virtual void                    Forward(VecTensorType const &inputs, TensorType &output) = 0;
  virtual std::vector<TensorType> Backward(VecTensorType const &inputs,
                                           TensorType const &   error_signal)                 = 0;
  /*
   * ComputeOutputShape is usually expensive function and should be used only for initialisation or
   * in ASSERT. On Forward you can use output.shape() and on Backward there is error_signal.shape()
   */
  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const = 0;
  std::vector<SizeType>         ComputeDefaultOutputShape(VecShapesType const &input_shapes)
  {
    VecShapesType tensor_shapes = input_shapes;
    VecTensorType dummies;
    for (auto &shape : tensor_shapes)
    {
      dummies.push_back(std::make_shared<TensorType>(shape));
    }
    default_output_shape_ = ComputeOutputShape(dummies);
    return default_output_shape_;
  }

  virtual std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() = 0;

  Ops() = default;

  explicit Ops(OpsSaveableParams const &sp)
  {
    is_training_ = sp.is_training;
  }

  virtual std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) = 0;

  void SetTraining(bool is_training)
  {
    is_training_ = is_training;
  }

  bool IsTraining()
  {
    return is_training_;
  }

  virtual ChargeAmount OpForwardCost(VecShapesType const &input_shapes)
  {
    FETCH_UNUSED(input_shapes);
    std::cout << __PRETTY_FUNCTION__ << std::endl
              << "   ^^ not-implemented cost calc called! returned 0." << std::endl;
    return 0;  // TODO(VH): make me a pure virtual.
  }

  void SetDefaultOutputShape(std::vector<SizeType> const &new_shape)
  {
    default_output_shape_ = new_shape;
  }

  virtual std::vector<SizeType> DefaultOutputShape()
  {
    return default_output_shape_;
  }

protected:
  bool is_training_ = true;

  std::vector<SizeType> default_output_shape_{};

  void PrintMyOutputShape()
  {
    std::vector<SizeType> const out_shape = this->DefaultOutputShape();
    std::cout << " (out shape ";
    if (out_shape.empty())
    {
      std::cout << "?? )";
      return;
    }
    std::cout << "{";
    for (auto const &dim : out_shape)
    {
      std::cout << " " << dim;
    }
    std::cout << " })";
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
