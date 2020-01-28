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

#include "ml/utilities/graph_builder.hpp"

namespace fetch {
namespace serializers {

/**
 * serializer for abs saveable params
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<ml::ops::Ops<TensorType>, D>
{
  using Type       = ml::ops::Ops<TensorType>;
  using DriverType = D;

  static uint8_t const OP_TYPE            = 1;
  static uint8_t const IS_TRAINING        = 2;
  static uint8_t const BATCH_INPUT_SHAPES = 3;
  static uint8_t const BATCH_OUTPUT_SHAPE = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &op)
  {
    auto map = map_constructor(1);

    map.Append(OP_TYPE, static_cast<uint16_t>(op.OperationType()));
    map.Append(IS_TRAINING, op.is_training_);
    map.Append(BATCH_INPUT_SHAPES, op.batch_input_shapes_);
    map.Append(BATCH_OUTPUT_SHAPE, op.batch_output_shape_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &op)
  {
    uint16_t op_type;
    map.ExpectKeyGetValue(OP_TYPE, op_type);
    op.op_type_ = static_cast<ml::OpType>(op_type);

    map.ExpectKeyGetValue(IS_TRAINING, op.is_training_);
    map.ExpectKeyGetValue(BATCH_INPUT_SHAPES, op.batch_input_shapes_);
    map.ExpectKeyGetValue(BATCH_OUTPUT_SHAPE, op.batch_output_shape_);
  }
};

}  // namespace serializers
}  // namespace fetch
