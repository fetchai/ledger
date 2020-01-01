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

#include "math/tensor/tensor.hpp"
#include "math/top_k.hpp"
#include "ml/exceptions/exceptions.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class TopK : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType     = T;
  using SizeType       = fetch::math::SizeType;
  using TensorSizeType = fetch::math::Tensor<SizeType>;
  using DataType       = typename TensorType::Type;
  using ArrayPtrType   = std::shared_ptr<TensorType>;
  using VecTensorType  = typename Ops<T>::VecTensorType;
  using SPType         = OpTopKSaveableParams<T>;
  using MyType         = TopK<TensorType>;

  /**
   * TopK function based on tf.top_k
   * @param k number of k highest numbers to be outputed
   * @param sorted TRUE=descending order, FALSE=ascending order
   */
  explicit TopK(SizeType k, bool sorted = true)
    : k_(k)
    , sorted_(sorted)
  {}

  explicit TopK(SPType const &sp)
    : Ops<T>(sp)
  {
    k_      = sp.k;
    sorted_ = sp.sorted;
  }

  ~TopK() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.k      = k_;
    sp.sorted = sorted_;

    return std::make_shared<SPType>(sp);
  }

  std::shared_ptr<Ops<TensorType>> MakeSharedCopy(std::shared_ptr<Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

    return copyshare;
  }

  /**
   * Returns array of k-highest values
   * for input array of shape [x,n] and value k, return array would be of shape [k,n]
   * Implementation based on tf.math.top_k
   * Updates indices array with indices of k highest values from input array
   * @param inputs input tensor
   * @param output return tensor
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);

    // Only 2D input is supported
    assert(inputs.at(0)->shape().size() == 2);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    UpdateIndices(inputs);

    fetch::math::TopK<TensorType, TensorSizeType>(output, indices_, *(inputs.at(0)), k_, axis_,
                                                  sorted_);
  }

  /**
   * Error signal is propagated to k largest nodes from input tensor
   * Forward needs to be called first to initialise indices array
   * @param inputs input tensor
   * @param error_signal
   * @return return signal tensor of same size as input tensor
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);

    // Only 2D input is supported
    assert(inputs.at(0)->shape().size() == 2);

    // Forward needs to be run first
    assert(indices_.size() != 0);

    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    TensorType ret_signal(inputs.at(0)->shape());

    for (SizeType i{0}; i < error_signal.shape().at(0); i++)
    {
      for (SizeType j{0}; j < error_signal.shape().at(1); j++)
      {
        ret_signal.At(indices_.At(i, j), j) = error_signal.At(i, j);
      }
    }

    return {ret_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    assert(inputs.size() == 1);

    std::vector<SizeType> ret_shape = inputs.at(0)->shape();

    if (ret_shape.size() > 1)
    {
      ret_shape.at(ret_shape.size() - 2) = k_;
    }
    else
    {
      ret_shape.at(ret_shape.size() - 1) = k_;
    }

    return ret_shape;
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_TOP_K;
  }
  static constexpr char const *DESCRIPTOR = "TopK";

private:
  SizeType k_;
  // For 2D input tensor we use data axis
  SizeType       axis_ = 0;
  bool           sorted_;
  TensorSizeType indices_;

  void UpdateIndices(VecTensorType const &inputs)
  {
    std::vector<SizeType> ret_shape = ComputeOutputShape(inputs);
    if (indices_.shape() != ret_shape)
    {
      indices_ = TensorSizeType(ret_shape);
    }
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
