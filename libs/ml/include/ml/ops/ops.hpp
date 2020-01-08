#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "math/tensor/tensor.hpp"
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

  /**
   * @brief ComputeBatchOutputShape is expensive function and should be used only for initialisation
   * or in ASSERT. On Forward you can use output.shape() and on Backward there is
   * error_signal.shape()
   * @param input_shapes
   * @return
   */
  std::vector<SizeType> ComputeBatchOutputShape(ShapeVector const &input_shapes)
  {
    ShapeVector   tensor_shapes = input_shapes;
    VecTensorType dummies;
    for (auto &shape : tensor_shapes)
    {
      dummies.push_back(std::make_shared<TensorType>(shape));
    }
    SetBatchOutputShape(ComputeOutputShape(dummies));
    SetBatchInputShapes(input_shapes);
    return batch_output_shape_;
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

  virtual void SetBatchOutputShape(Shape const &new_shape)
  {
    batch_output_shape_ = new_shape;
  }

  virtual void SetBatchInputShapes(ShapeVector const &new_shapes)
  {
    batch_input_shapes_ = new_shapes;
  }

  /**
   * @brief BatchOutputShape returns an output shape of the layer, if a singluar slice of an input
   * data is given (e.g. batch size == 1)
   */
  virtual Shape const &BatchOutputShape() const
  {
    return batch_output_shape_;
  }

  /**
   * @brief BatchInputShapes returns a vector of shapes, that describes expected input
   * slice shapes (e.g. when batch size of input data is 1)
   */
  virtual ShapeVector const &BatchInputShapes() const
  {
    return batch_input_shapes_;
  }

  /// OOP polymorphic wrapper around each Ops/Layer OpCode() static method.
  virtual OpType OperationType() const  // TODO(VH): make a pure virtual.
  {
    std::cout << "Error: call to unexisting OperationType implementation! returned None."
              << std::endl;
    return OpType::NONE;
  }

  /// OOP polymorphic wrapper around each Ops/Layer DESCRIPTOR.
  virtual char const *Descriptor() const  // TODO(VH): make a pure virtual.
  {
    std::cout << "Error: call to unexisting OperationType implementation! returned None."
              << std::endl;
    return "UNKNOWN";
  }

  /// Should be called after shape linking in Graph to complete all initialisations, that depends
  /// on layer shapes (like trainable parameter tensors init. and so on)
  virtual void CompleteInitialisation()
  {
    // Empty deafult implementation for non-trainable Ops.
  }

  // TODO(VH): extract to a free function.
  std::string OutputShapeAsString()
  {
    std::vector<SizeType> const out_shape = this->BatchOutputShape();
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

  // TODO(VH): extract to a free function.
  std::string InputShapesAsString()
  {
    std::vector<std::vector<SizeType>> const in_shapes = this->BatchInputShapes();
    std::stringstream                        ss;
    ss << " (in ";
    if (in_shapes.empty())
    {
      ss << "[[??]] )";
      return ss.str();
    }
    ss << "[";
    for (auto const &shape : in_shapes)
    {
      ss << "[";
      for (auto const &dim : shape)
      {
        ss << " " << dim;
      }
      ss << " ]";
    }
    ss << "])";
    return ss.str();
  }

protected:
  bool is_training_ = true;

  ShapeVector batch_input_shapes_{};
  Shape       batch_output_shape_{};
  // TODO(VH): ^^ impl. serialisation of new fields.

  // TODO(VH): extract to a free function.
  static fetch::math::SizeType TotalElementsIn(std::vector<fetch::math::SizeVector> const &shapes)
  {
    if (shapes.empty())
    {
      return 0;
    }
    fetch::math::SizeType total_elements = 1;
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
