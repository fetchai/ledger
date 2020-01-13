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

#include "ml/core/subgraph.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/weights.hpp"
#include "ml/regularisers/regularisation.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class FullyConnected : public SubGraph<T>
{
public:
  using TensorType    = T;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using DataType      = typename TensorType::Type;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerFullyConnectedSaveableParams<TensorType>;

  using OpPtrType      = std::shared_ptr<fetch::ml::ops::Ops<TensorType>>;
  using GraphType      = fetch::ml::Graph<TensorType>;
  using GraphPtrType   = std::shared_ptr<GraphType>;
  using WeightsType    = fetch::ml::ops::Weights<TensorType>;
  using WeightsPtrType = std::shared_ptr<WeightsType>;

  /**
   * @brief FullyConnected default constructor without parameters is used to create an empty
   * object during deserialisation. After deserialisation the object is treated as an initialised
   * one.
   */
  FullyConnected()
    : SubGraph<T>()
    , is_initialised_(true)
  {}

  /**
   * Normal fully connected layer constructor
   * @param in
   * @param out
   * @param activation_type
   * @param regulariser
   * @param regularisation_rate
   * @param init_mode
   */
  FullyConnected(SizeType in, SizeType out,
                 details::ActivationType       activation_type = details::ActivationType::NOTHING,
                 fetch::ml::RegularisationType regulariser = fetch::ml::RegularisationType::NONE,
                 DataType                      regularisation_rate = DataType{0},
                 WeightsInit                   init_mode           = WeightsInit::XAVIER_GLOROT,
                 bool                          time_distributed    = !TIME_DISTRIBUTED)
    : in_size_(in)
    , out_size_(out)
    , time_distributed_(time_distributed)
    , regulariser_(regulariser)
    , regularisation_rate_(regularisation_rate)
    , init_mode_(init_mode)
  {
    // get correct name for the layer
    std::string name = GetName();

    // start to set up the structure
    input_ = this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});
    // for non time distributed layer, flatten the input
    flattened_input_ = input_;
    if (!time_distributed_)
    {
      flattened_input_ =
          this->template AddNode<fetch::ml::ops::Flatten<TensorType>>(name + "_Flatten", {input_});
    }

    weights_ = this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights", {});

    std::string weights_matmul_ =
        this->template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
            name + "_MatrixMultiply", {weights_, flattened_input_});

    bias_ = this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Bias", {});

    std::string add_ = this->template AddNode<fetch::ml::ops::Add<TensorType>>(
        name + "_Add", {weights_matmul_, bias_});

    output_ =
        fetch::ml::details::AddActivationNode<T>(activation_type, this, name + "_Activation", add_);

    this->AddInputNode(input_);
    this->SetOutputNode(output_);

    // If inputs count is known, the initialisation can be completed immediately.
    if (in_size_ != AUTODETECT_INPUT_SHAPE)
    {
      if (time_distributed_)
      {
        this->batch_input_shapes_ = {{in_size_, 1, 1}};
      }
      else
      {
        this->batch_input_shapes_ = {{in_size_, 1}};
      }
      this->ComputeBatchOutputShape(this->batch_input_shapes_);
      CompleteInitialisation();
    }
  }

  void CompleteInitialisation() override
  {
    if (is_initialised_)
    {
      return;
    }

    assert(!this->batch_input_shapes_.empty());
    assert(!this->batch_output_shape_.empty());
    FETCH_LOG_INFO(Descriptor(), "-- Compiling sub-graph ... --");
    this->nodes_.at(input_)->SetBatchInputShapes(this->batch_input_shapes_);
    this->nodes_.at(input_)->SetBatchOutputShape(this->batch_input_shapes_.front());

    if (in_size_ == AUTODETECT_INPUT_SHAPE)
    {
      if (time_distributed_)
      {
        math::SizeVector const &first_input_shape = this->batch_input_shapes_.front();
        // An input size of a time-distributed layer is equal to a first dimension in
        // the input shape.
        in_size_ = first_input_shape.front();
      }
      else
      {
        // An input size of a non-time-distributed layer is equal to total elements
        // in input tensor, e.g. equal to a flattened input output size.
        this->nodes_.at(flattened_input_)
            ->GetOp()
            ->ComputeBatchOutputShape(this->batch_input_shapes_);
        in_size_ = this->nodes_.at(flattened_input_)->BatchOutputShape().front();
      }
    }
    out_size_ = this->batch_output_shape_.front();

    // At this point we know everything necessary to directly assign shapes to
    // leaf nodes such as Weights and Bias.
    this->nodes_.at(weights_)->SetBatchOutputShape({out_size_, in_size_});
    this->nodes_.at(bias_)->SetBatchOutputShape(this->batch_output_shape_);

    // initialize weight with specified method.
    TensorType weights_data(std::vector<SizeType>({out_size_, in_size_}));
    fetch::ml::ops::Weights<TensorType>::Initialise(weights_data, in_size_, out_size_, init_mode_);
    this->SetInput(weights_, weights_data);

    // initialize bias with right shape and set to all zero.
    TensorType bias_data;
    if (time_distributed_)
    {
      bias_data = TensorType(std::vector<SizeType>({out_size_, 1, 1}));
    }
    else
    {
      bias_data = TensorType(std::vector<SizeType>({out_size_, 1}));
    }
    this->SetInput(bias_, bias_data);

    this->SetRegularisation(fetch::ml::details::CreateRegulariser<T>(regulariser_),
                            regularisation_rate_);
    this->Compile();
    FETCH_LOG_INFO(Descriptor(), "-- Sub-graph compiled. --");
    is_initialised_ = true;
  }

  OpPtrType MakeSharedCopy(OpPtrType me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);  // used for compatability

    auto copyshare = std::make_shared<FullyConnected<TensorType>>();

    copyshare->time_distributed_ = time_distributed_;
    copyshare->in_size_          = in_size_;
    copyshare->out_size_         = out_size_;

    SubGraph<TensorType>::InsertSharedCopy(copyshare);

    return copyshare;
  }

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto ret = std::make_shared<SPType>();
    // get base class saveable params
    std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

    // assign base class saveable params to ret
    auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
    auto sg_ptr2 = std::static_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
    *sg_ptr2     = *sg_ptr1;

    // asign layer specific params
    ret->in_size          = in_size_;
    ret->out_size         = out_size_;
    ret->time_distributed = time_distributed_;

    return ret;
  }

  void SetOpSaveableParams(SPType const &sp)
  {
    // assign layer specific params
    in_size_          = sp.in_size;
    out_size_         = sp.out_size;
    time_distributed_ = sp.time_distributed;
  }

  math::SizeVector ComputeOutputShape(VecTensorType const &inputs) const override
  {
    if (!time_distributed_)
    {
      SizeType total_in_size = 1;
      for (std::size_t i = 0; i < inputs.front()->shape().size() - 1; i++)
      {
        total_in_size *= inputs.front()->shape(i);
      }
      assert((this->in_size_ == AUTODETECT_INPUT_SHAPE) || (total_in_size == this->in_size_));
      return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 1)};
    }

    assert(inputs.front()->shape().size() == 3);
    assert(inputs.front()->shape(0) == in_size_);
    return {this->out_size_, inputs.front()->shape(inputs.front()->shape().size() - 2),
            inputs.front()->shape(inputs.front()->shape().size() - 1)};
  }

  math::SizeVector ComputeBatchOutputShape(
      std::vector<math::SizeVector> const &input_shapes) override
  {
    if (time_distributed_)
    {
      assert((this->in_size_ == AUTODETECT_INPUT_SHAPE) ||
             (input_shapes.front().at(0) == in_size_));

      this->SetBatchInputShapes(input_shapes);
      if (input_shapes.front().size() == 3)
      {
        this->SetBatchOutputShape({this->out_size_, input_shapes.front().at(1), 1});
      }
      else
      {
        this->SetBatchOutputShape({this->out_size_, 1, 1});
      }
      return this->batch_output_shape_;
    }

    this->SetBatchInputShapes(input_shapes);
    this->SetBatchOutputShape({this->out_size_, 1});
    return this->batch_output_shape_;
  }

  static constexpr OpType OpCode()
  {
    return fetch::ml::OpType::LAYER_FULLY_CONNECTED;
  }

  static constexpr char const *DESCRIPTOR = "FullyConnected";

  OpType OperationType() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return this->OpCode();
  }
  char const *Descriptor() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return DESCRIPTOR;
  }

  static constexpr SizeType AUTODETECT_INPUT_SHAPE = 0;
  static constexpr bool     TIME_DISTRIBUTED       = true;

private:
  // Saveable params
  SizeType in_size_          = fetch::math::numeric_max<SizeType>();
  SizeType out_size_         = fetch::math::numeric_max<SizeType>();
  bool     time_distributed_ = false;

  // Non-saveable params
  bool                          is_initialised_ = false;
  std::string                   input_;
  std::string                   flattened_input_;
  std::string                   weights_;
  std::string                   bias_;
  std::string                   output_;
  fetch::ml::RegularisationType regulariser_;
  DataType                      regularisation_rate_;
  WeightsInit                   init_mode_;

  std::string GetName()
  {
    std::string name = DESCRIPTOR;
    if (time_distributed_)
    {
      name = "TimeDistributed_" + name;
    }
    return name;
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
