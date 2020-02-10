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

namespace serialisers {

/**
 * serialiser for ModelConfig
 * @tparam TensorType
 */
template <typename DataType, typename D>
struct MapSerialiser<fetch::ml::model::ModelConfig<DataType>, D>
  : MapSerialiserBoilerplate<
        fetch::ml::model::ModelConfig<DataType>, D,
        serialiseD_STRUCT_FIELD(1, fetch::ml::model::ModelConfig<DataType>::early_stopping),
        serialiseD_STRUCT_FIELD(2, fetch::ml::model::ModelConfig<DataType>::test),
        serialiseD_STRUCT_FIELD(3, fetch::ml::model::ModelConfig<DataType>::patience),
        serialiseD_STRUCT_FIELD(4, fetch::ml::model::ModelConfig<DataType>::min_delta),
        serialiseD_STRUCT_FIELD(6, fetch::ml::model::ModelConfig<DataType>::learning_rate_param),
        serialiseD_STRUCT_FIELD(7, fetch::ml::model::ModelConfig<DataType>::batch_size),
        serialiseD_STRUCT_FIELD(8, fetch::ml::model::ModelConfig<DataType>::subset_size),
        serialiseD_STRUCT_FIELD(9, fetch::ml::model::ModelConfig<DataType>::print_stats)>
{
};

}  // namespace serialisers
}  // namespace fetch
