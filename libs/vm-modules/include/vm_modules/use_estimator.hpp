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

#include "meta/callable/callable_traits.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm_modules {

/*
  Converts a member function `estimator` of an estimator object `EstimatorType` to a free function
  intended to be used as a charge estimation for a VM object member function.

  - `EstimatorType` : the type of the estimator object, should define an `ObjectType` that
  corresponds to the type of the vm object it is providing estimators for

  - `estimator`     : should be a member function of the class `EstimatorType`

  - `EtchArgs...`   : list of arguments of of the `estimator` member function as well as the vm
  object one

*/

template <typename EstimatorType, typename... EtchArgs>
vm::ChargeEstimator<vm::Ptr<typename EstimatorType::VMObjectType>, EtchArgs...> use_estimator(
    vm::ChargeAmount (EstimatorType::*estimator)(EtchArgs... args))
{
  return
      [estimator](vm::Ptr<typename EstimatorType::VMObjectType> context, EtchArgs... args) -> vm::ChargeAmount {
        return (context->Estimator().*estimator)(args...);
      };
}

}  // namespace vm_modules
}  // namespace fetch
