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

#include "ml/regularisers/l1_regulariser.hpp"
#include "ml/regularisers/l2_regulariser.hpp"
#include "ml/regularisers/regulariser.hpp"
//#include "regularisation.hpp"

#include <memory>
#include <stdexcept>

namespace fetch {
namespace ml {
namespace details {

enum class RegularisationType
{
  NONE,
  L1,
  L2,
};

template <class T>
std::shared_ptr<fetch::ml::regularisers::Regulariser<T>> CreateRegulariser(RegularisationType type)
{
  using RegType    = fetch::ml::regularisers::Regulariser<T>;
  using RegPtrType = std::shared_ptr<RegType>;

  RegPtrType ret;
  switch (type)
  {
  case RegularisationType::NONE:
    // No regulariser
    break;

  case RegularisationType::L1:
    ret.reset(new fetch::ml::regularisers::L1Regulariser<T>());
    break;

  case RegularisationType::L2:
    ret.reset(new fetch::ml::regularisers::L2Regulariser<T>());
    break;

  default:
    throw std::runtime_error("Unknown regulariser type");
  }
  return ret;
}
}  // namespace details
}  // namespace ml
}  // namespace fetch
