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

#include "test_types.hpp"

#include "ml/serialisers/ml_types.hpp"

#include "gtest/gtest.h"

#include <memory>

namespace serializer_test_utils {

/// helper functions

/**
 *
 * @tparam SPType
 * @tparam TensorType
 * @tparam LayerType
 * @param layer
 * @param mode - if true, LayerType should be a Layer, otherwise an Op
 * @return
 */
template <typename SPType, typename TensorType, typename LayerType>
std::shared_ptr<SPType> SerialiseDeserialiseBuild(LayerType &layer)
{
  // extract saveparams
  auto sp = layer.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  ::fetch::serializers::MsgPackSerializer b{};
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  return dsp2;
}

template <typename DataType, typename TensorType, typename LayerType>
void TestLayerPredictionsEqual(LayerType &layer, LayerType &layer2, TensorType const &input,
                               std::string const &input_name, std::string const &output_name,
                               TensorType const &prediction0)
{
  // test equality
  layer.SetInput(input_name, input);
  auto prediction = layer.Evaluate(output_name, true);

  layer2.SetInput(input_name, input);
  TensorType prediction2 = layer2.Evaluate(output_name, true);

  EXPECT_TRUE(prediction.AllClose(prediction2, ::fetch::math::function_tolerance<DataType>(),
                                  ::fetch::math::function_tolerance<DataType>()));

  // sanity check - serialisation should not affect initial prediction
  ASSERT_TRUE(prediction0.AllClose(prediction, ::fetch::math::function_tolerance<DataType>(),
                                   ::fetch::math::function_tolerance<DataType>()));
}

}  // namespace serializer_test_utils
