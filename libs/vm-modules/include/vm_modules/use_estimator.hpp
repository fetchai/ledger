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

#include "meta/callable/callable_traits.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm_modules {

namespace internal {

template <typename... EtchArgs>
struct EstimatorFromMemberFunction
{
  template <typename Callable>
  static auto MakeEstimator(Callable &&estimator)
  {
    using EstimatorType = typename meta::CallableTraits<Callable>::OwningType;
    return [estimator](vm::Ptr<typename EstimatorType::VMObjectType> context,
                       EtchArgs... args) -> vm::ChargeAmount {
      return meta::Invoke(estimator, context->Estimator(), args...);
    };
  }
};

}  // namespace internal

/*
  Converts a member function `estimator` of an estimator object to a free function
  intended to be used as a charge estimation for a VM object member function.

  - `estimator`   : pointer of the estimator object member function to be used

*/

template <typename Callable>
auto UseEstimator(Callable &&estimator)
{
  using EtchArgs = typename meta::CallableTraits<Callable>::ArgsTupleType;
  return meta::UnpackTuple<EtchArgs, internal::EstimatorFromMemberFunction>::MakeEstimator(
      std::forward<Callable>(estimator));
}

}  // namespace vm_modules
}  // namespace fetch
