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
#include "meta/vm_types.hpp"
#include "ml/ops/estimation/charge_constants.hpp"
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
  using Shape         = fetch::math::SizeVector;
  using ShapeVector   = std::vector<Shape>;

  virtual ~Ops() = default;

  virtual void                    Forward(VecTensorType const &inputs, TensorType &output) = 0;
  virtual std::vector<TensorType> Backward(VecTensorType const &inputs,
                                           TensorType const &   error_signal)                 = 0;
  /*
   * ComputeOutputShape is usually expensive function and should be used only for initialisation or
   * in ASSERT. On Forward you can use output.shape() and on Backward there is error_signal.shape()
   */
  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const = 0;
  std::vector<SizeType>         ComputeSliceOutputShape(ShapeVector const &input_shapes)
  {
    ShapeVector   tensor_shapes = input_shapes;
    VecTensorType dummies;
    for (auto &shape : tensor_shapes)
    {
      dummies.push_back(std::make_shared<TensorType>(shape));
    }
    slice_output_shape_ = ComputeOutputShape(dummies);
    return slice_output_shape_;
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

  virtual fetch::vm::ChargeAmount OpForwardCost(ShapeVector const &input_shapes)
  {
    FETCH_UNUSED(input_shapes);
    FETCH_LOG_WARN("Ops", " not-implemented OpForwardCost() called! returned 0.");
    return 0;  // TODO(VH): make me a pure virtual.
  }

  void SetSliceOutputShape(Shape const &new_shape)
  {
    slice_output_shape_ = new_shape;
  }

  void SetExpectedSliceInputShapes(ShapeVector const &new_shapes)
  {
    expected_slice_input_shapes_ = new_shapes;
  }

  virtual Shape SliceOutputShape() const
  {
    return slice_output_shape_;
  }

  virtual ShapeVector ExpectedSliceInputShapes() const
  {
    return expected_slice_input_shapes_;
  }

  virtual OpType OperationType() const
  {
    return OpType::NONE;  // TODO(VH): make pure virtual.
  }

protected:
  bool is_training_ = true;

  ShapeVector expected_slice_input_shapes_{};  // TODO(VH): impl. filling it on compilation.
  Shape       slice_output_shape_{};

  std::string OutputShapeAsString()
  {
    std::vector<SizeType> const out_shape = this->SliceOutputShape();
    std::stringstream           ss;
    ss << " (out ";
    if (out_shape.empty())
    {
      ss << "[??] )";
      return ss.str();
    }
    ss << "[";
    for (auto const &dim : out_shape)
    {
      ss << " " << dim;
    }
    ss << " ])";
    return ss.str();
  }

  static SizeType TotalElementsIn(ShapeVector const &shapes)
  {
    if (shapes.empty())
    {
      return 0;
    }
    SizeType total_elements = 1;
    for (auto const &shape : shapes)
    {
      for (auto const &dimension : shape)
      {
        // TODO(VH): handle a bad case with a dim of size 0.
        total_elements *= dimension;
      }
    }
    return total_elements;
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
