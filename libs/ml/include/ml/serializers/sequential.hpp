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

namespace fetch {

namespace ml {
namespace model {
template <typename TensorType>
class Sequential;
template <typename DataType>
class Model;
}  // namespace model
}  // namespace ml

namespace serializers {
/**
 * serializer for Sequential model
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::model::Sequential<TensorType>, D>
  : MapSerializerBoilerplate<
        ml::model::Sequential<TensorType>, D, SimplySerializedAs<1, ml::model::Model<TensorType>>,
        EXPECTED_KEY_MEMBER(2, ml::model::Sequential<TensorType>::layer_count_),
        EXPECTED_KEY_MEMBER(3, ml::model::Sequential<TensorType>::prev_layer_)>
{
};

}  // namespace serializers
}  // namespace fetch
