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

#include "ml/ops/weights.hpp"

namespace fetch {
namespace serializers {

template< typename V, typename D >
struct ArraySerializer< ml::StateDict< V >, D >
{
public:
  using Type = memory::Array< V >;
  using DriverType = D;

  template< typename Constructor >
  static void Serialize(Constructor & array_constructor, Type const & sd)
  {
    auto array = array_constructor(2);
    array.Append(*sd.weights_);
    array.Append(sd.dict_);    
  }

  template< typename ArrayDeserializer >
  static void Deserialize(ArrayDeserializer & array, Type & output)
  {
    output.weights_ = std::make_shared<V>();
    array.GetNextValue(*output.weights_);
    array.GetNextValue(output.dict_);
  }  
};

}  // namespace serializers
}  // namespace fetch
