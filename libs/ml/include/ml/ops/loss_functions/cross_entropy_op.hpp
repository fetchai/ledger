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

#include <cassert>
#include <memory>
#include <vector>

#include "math/fundamental_operators.hpp"
#include "math/ml/activation_functions/sigmoid.hpp"
#include "math/ml/activation_functions/softmax.hpp"
#include "math/ml/loss_functions/cross_entropy.hpp"


namespace fetch {
    namespace ml {
        namespace ops {

            template <class T>
            class CrossEntropyOp : public Ops<T>
            {
            public:
                using ArrayType     = T;
                using DataType      = typename ArrayType::Type;
                using SizeType      = typename ArrayType::SizeType;
                using ArrayPtrType  = std::shared_ptr<ArrayType>;
                using VecTensorType = typename Ops<T>::VecTensorType;

                CrossEntropyOp()          = default;
                virtual ~CrossEntropyOp() = default;

                virtual void Forward(VecTensorType const &inputs, ArrayType &output)
                {
                    assert(inputs.size() == 2);
                    assert(inputs.at(0).get().size() == inputs.at(1).get().size());

                   output(0,0)=fetch::math::CrossEntropyLoss(inputs[0].get(), inputs[1].get());
                }

                virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                                        ArrayType const &    error_signal)
                {
                    assert(inputs.size() == 2);
                    assert(inputs[0].get().size() == inputs[1].get().size());
                    assert(inputs[0].get().shape().size() == 2);

                    ArrayType ret;
                    if (inputs[0].get().shape().at(0) == 1)  // not one-hot
                    {
                        ret = fetch::math::Sigmoid(inputs[0].get());
                        fetch::math::Subtract(ret, inputs[1].get(), ret);
                        fetch::math::Multiply(ret, inputs[0].get(), ret);
                    }
                    else if (inputs[0].get().shape().size())  // one-hot
                    {
                        ret = fetch::math::Softmax(inputs[0].get(), 1);
                        fetch::math::Divide(inputs[1].get(), ret, ret);
                        fetch::math::Multiply(DataType(-1), ret, ret);
                    }

                    // chain rule
                    fetch::math::Multiply(ret, error_signal, ret);


                    return {ret, ArrayType(inputs.at(1).get().shape())};
                }

                std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const
                {
                    (void)inputs;
                    return {1, 1};
                }

                static constexpr char const *DESCRIPTOR = "CrossEntropyOp";
            };

        }  // namespace ops
    }  // namespace ml
}  // namespace fetch
