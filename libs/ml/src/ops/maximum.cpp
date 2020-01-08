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
#include "ml/ops/maximum.hpp"

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
std::shared_ptr<OpsSaveableParams> Maximum<T>::GetOpSaveableParams()
{
  SPType sp{};
  return std::make_shared<SPType>(sp);
}

template <class TensorType>
std::shared_ptr<fetch::ml::ops::Ops<TensorType>> Maximum<TensorType>::MakeSharedCopy(
    std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me)
{
  FETCH_UNUSED(me);
  assert(me.get() == this);

  auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

  return copyshare;
}

template <class T>
void Maximum<T>::Forward(const VecTensorType &inputs, TensorType &output)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());
  assert(output.shape() == this->ComputeOutputShape(inputs));

  fetch::math::Maximum((*inputs.at(0)), (*inputs.at(1)), output);
}

template <class TensorType>
std::vector<TensorType> Maximum<TensorType>::Backward(const VecTensorType &inputs,
                                                      const TensorType &   error_signal)
{
  assert(inputs.size() == 2);
  assert(inputs.at(0)->size() == inputs.at(1)->size());
  assert(error_signal.size() == inputs.at(1)->size());

  TensorType return_signal_1(inputs.at(0)->shape());
  TensorType return_signal_2(inputs.at(1)->shape());

  auto a_it   = inputs.at(0)->cbegin();
  auto b_it   = inputs.at(1)->cbegin();
  auto err_it = error_signal.cbegin();
  auto r_1_it = return_signal_1.begin();
  auto r_2_it = return_signal_2.begin();
  while (a_it.is_valid())
  {
    if ((*a_it) > (*b_it))
    {
      *r_1_it = *err_it;
    }
    else
    {
      *r_2_it = *err_it;
    }

    ++a_it;
    ++b_it;
    ++err_it;
    ++r_1_it;
    ++r_2_it;
  }

  return {return_signal_1, return_signal_2};
}

template <class T>
std::vector<fetch::math::SizeType> Maximum<T>::ComputeOutputShape(const VecTensorType &inputs) const
{
  return inputs.front()->shape();
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
