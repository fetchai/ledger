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

#include "ml/optimisation/adagrad_optimiser.hpp"
#include "ml/optimisation/adam_optimiser.hpp"
#include "ml/optimisation/momentum_optimiser.hpp"
#include "ml/optimisation/optimiser.hpp"
#include "ml/optimisation/rmsprop_optimiser.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"

namespace fetch {
namespace ml {
namespace optimisers {

enum class OptimiserType
{
  ADAGRAD,
  ADAM,
  MOMENTUM,
  RMSPROP,
  SGD
};

template <class T, typename... Params>
bool AddOptimiser(OptimiserType type, std::shared_ptr<Optimiser<T>> &optimiser_ptr,
                  Params... params)
{
  switch (type)
  {
  case OptimiserType::ADAGRAD:
  {
    optimiser_ptr.reset(new fetch::ml::optimisers::AdaGradOptimiser<T>(params...));
    return true;
  }
  case OptimiserType::ADAM:
  {
    optimiser_ptr.reset(new fetch::ml::optimisers::AdamOptimiser<T>(params...));
    return true;
  }
  case OptimiserType::MOMENTUM:
  {
    optimiser_ptr.reset(new fetch::ml::optimisers::MomentumOptimiser<T>(params...));
    return true;
  }
  case OptimiserType::RMSPROP:
  {
    optimiser_ptr.reset(new fetch::ml::optimisers::RMSPropOptimiser<T>(params...));
    return true;
  }
  case OptimiserType::SGD:
  {
    optimiser_ptr.reset(new fetch::ml::optimisers::SGDOptimiser<T>(params...));
    return true;
  }
  default:
  {
    return false;
  }
  }
}

}  // namespace optimisers
}  // namespace ml
}  // namespace fetch
