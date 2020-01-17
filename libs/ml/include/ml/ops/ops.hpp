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
   * @brief ComputeBatchOutputShape is expensive function and should be used only for
   * initialisation. This is a default naive implementation that is intended to be overriden in
   * derived classes for better efficiency.
   * @param input_shapes
   * @return
   */
  virtual std::vector<SizeType> ComputeBatchOutputShape(ShapeVector const &input_shapes)
  {
    VecTensorType dummies;
    dummies.reserve(input_shapes.size());
    for (auto &shape : input_shapes)
    {
      dummies.emplace_back(std::make_shared<TensorType>(shape));
    }
    SetBatchOutputShape(ComputeOutputShape(dummies));
    SetBatchInputShapes(input_shapes);
    return batch_output_shape_;
  }

  virtual std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() = 0;

  Ops() = default;

  explicit Ops(OpsSaveableParams const &sp)
    : is_training_(sp.is_training)
    , batch_input_shapes_(sp.batch_input_shapes)
    , batch_output_shape_(sp.batch_output_shape)
  {}

  virtual std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) = 0;

  void SetTraining(bool is_training)
  {
    is_training_ = is_training;
  }

  inline bool IsTraining() const
  {
    return is_training_;
  }

  inline void SetBatchOutputShape(Shape const &new_shape)
  {
    batch_output_shape_ = new_shape;
  }

  inline void SetBatchInputShapes(ShapeVector const &new_shapes)
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
  inline ShapeVector const &BatchInputShapes() const
  {
    return batch_input_shapes_;
  }

  /// OOP polymorphic wrapper around each Ops/Layer OpCode() static method.
  virtual OpType OperationType() const  // TODO(ML-466): make a pure virtual.
  {
    // TODO(ML-466): std::cout usage will be eliminated in pure virtual method.
    std::cout << "Error: call to unexisting OperationType() implementation! returned None."
              << std::endl;
    return OpType::NONE;
  }

  /// OOP polymorphic wrapper around each Ops/Layer DESCRIPTOR.
  virtual char const *Descriptor() const  // TODO(ML-466): make a pure virtual.
  {
    // TODO(ML-466): std::cout usage will be eliminated in pure virtual method.
    std::cout << "Error: call to unexisting Descriptor() implementation! returned UNKNOWN."
              << std::endl;
    return "UNKNOWN";
  }

  /// Should be called after shape linking in Graph to complete all initialisations, that depends
  /// on layer shapes (like trainable parameter tensors init. and so on)
  virtual void CompleteConstruction()
  {
    // Empty deafult implementation for non-trainable Ops.
  }

protected:
  bool is_training_ = true;

  ShapeVector batch_input_shapes_{};
  Shape       batch_output_shape_{};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
