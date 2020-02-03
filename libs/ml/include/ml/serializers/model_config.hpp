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

template <typename DataType>
struct ModelConfig;

}  // namespace model
}  // namespace ml

namespace serializers {

/**
 * serializer for ModelConfig
 * @tparam TensorType
 */
template <typename DataType, typename D>
struct MapSerializer<fetch::ml::model::ModelConfig<DataType>, D>
  : MapSerializerBoilerplate<
        fetch::ml::model::ModelConfig<DataType>, D,
        EXPECTED_KEY_MEMBER(1, fetch::ml::model::ModelConfig<DataType>::early_stopping),
        EXPECTED_KEY_MEMBER(2, fetch::ml::model::ModelConfig<DataType>::test),
        EXPECTED_KEY_MEMBER(3, fetch::ml::model::ModelConfig<DataType>::patience),
        EXPECTED_KEY_MEMBER(4, fetch::ml::model::ModelConfig<DataType>::min_delta),
        EXPECTED_KEY_MEMBER(6, fetch::ml::model::ModelConfig<DataType>::learning_rate_param),
        EXPECTED_KEY_MEMBER(7, fetch::ml::model::ModelConfig<DataType>::batch_size),
        EXPECTED_KEY_MEMBER(8, fetch::ml::model::ModelConfig<DataType>::subset_size),
        EXPECTED_KEY_MEMBER(9, fetch::ml::model::ModelConfig<DataType>::print_stats)>
{
};

}  // namespace serializers
}  // namespace fetch
