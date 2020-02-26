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

#include "core/random.hpp"
#include "core/serializers/group_definitions.hpp"
#include "math/base_types.hpp"
#include "ml/meta/ml_type_traits.hpp"

#include <vector>

namespace fetch {
namespace ml {
namespace dataloaders {

enum class DataLoaderMode : uint16_t
{
  TRAIN,
  VALIDATE,
  TEST,
};

template <typename TensorType>
class DataLoader
{
public:
  using SizeType   = fetch::math::SizeType;
  using SizeVector = fetch::math::SizeVector;
  using ReturnType = std::pair<TensorType, std::vector<TensorType>>;

  explicit DataLoader() = default;

  virtual ~DataLoader() = default;

  virtual ReturnType GetNext() = 0;

  virtual bool       AddData(std::vector<TensorType> const &data, TensorType const &label) = 0;
  virtual ReturnType PrepareBatch(fetch::math::SizeType batch_size, bool &is_done_set);

  virtual SizeType Size() const                                                 = 0;
  virtual bool     IsDone() const                                               = 0;
  virtual void     Reset()                                                      = 0;
  virtual void     SetTestRatio(fixed_point::fp32_t new_test_ratio)             = 0;
  virtual void     SetValidationRatio(fixed_point::fp32_t new_validation_ratio) = 0;
  void             SetMode(DataLoaderMode new_mode);
  virtual bool     IsModeAvailable(DataLoaderMode mode) = 0;
  void             SetRandomMode(bool random_mode_state);
  void             SetSeed(SizeType seed = 123);

  template <typename X, typename D>
  friend struct fetch::serializers::MapSerializer;

  virtual LoaderType LoaderCode() = 0;

  std::pair<math::SizeVector, std::vector<math::SizeVector>> GetDataSize(
      SizeType const &batch_size = 0);

protected:
  virtual void              UpdateCursor()  = 0;
  std::shared_ptr<SizeType> current_cursor_ = std::make_shared<SizeType>(0);
  SizeType                  current_min_{};
  SizeType                  current_max_{};
  SizeType                  current_size_{};

  bool                                      random_mode_ = false;
  DataLoaderMode                            mode_        = DataLoaderMode::TRAIN;
  fetch::random::LaggedFibonacciGenerator<> rand{};

private:
  bool       size_not_set_ = true;
  ReturnType cur_training_pair_;
  ReturnType ret_pair_;

  void SetDataSize(std::pair<TensorType, std::vector<TensorType>> &ret_pair);
};

}  // namespace dataloaders
}  // namespace ml

namespace serializers {

/**
 * serializer for DataLoaderMode
 * @tparam TensorType
 */
template <typename D>
struct MapSerializer<ml::dataloaders::DataLoaderMode, D>
{
  using Type       = ml::dataloaders::DataLoaderMode;
  using DriverType = D;

  static uint8_t const OP_CODE = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &body)
  {
    auto map      = map_constructor(1);
    auto enum_val = static_cast<uint16_t>(body);
    map.Append(OP_CODE, enum_val);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &body)
  {
    uint16_t op_code_int = 0;
    map.ExpectKeyGetValue(OP_CODE, op_code_int);

    body = static_cast<Type>(op_code_int);
  }
};

/**
 * serializer for Dataloader
 * @tparam TensorType
 */
template <typename TensorType, typename D>
struct MapSerializer<fetch::ml::dataloaders::DataLoader<TensorType>, D>
{
  using Type       = fetch::ml::dataloaders::DataLoader<TensorType>;
  using DriverType = D;

  static uint8_t const RANDOM_MODE              = 1;
  static uint8_t const MODE                     = 2;
  static uint8_t const RAND_SEED                = 3;
  static uint8_t const RAND_BUFFER              = 4;
  static uint8_t const RAND_INDEX               = 5;
  static uint8_t const SIZE_NOT_SET             = 6;
  static uint8_t const CUR_TRAINING_PAIR_FIRST  = 7;
  static uint8_t const CUR_TRAINING_PAIR_SECOND = 8;
  static uint8_t const RET_PAIR_FIRST           = 9;
  static uint8_t const RET_PAIR_SECOND          = 10;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &sp)
  {
    auto map = map_constructor(10);

    map.Append(RANDOM_MODE, sp.random_mode_);
    map.Append(MODE, sp.mode_);
    map.Append(RAND_SEED, sp.rand.Seed());
    auto tmp = (sp.rand).GetBuffer();
    map.Append(RAND_BUFFER, tmp);
    map.Append(RAND_INDEX, sp.rand.GetIndex());
    map.Append(SIZE_NOT_SET, sp.size_not_set_);
    map.Append(CUR_TRAINING_PAIR_FIRST, sp.cur_training_pair_.first);
    map.Append(CUR_TRAINING_PAIR_SECOND, sp.cur_training_pair_.second);
    map.Append(RET_PAIR_FIRST, sp.ret_pair_.first);
    map.Append(RET_PAIR_SECOND, sp.ret_pair_.second);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &sp)
  {
    map.ExpectKeyGetValue(RANDOM_MODE, sp.random_mode_);
    map.ExpectKeyGetValue(MODE, sp.mode_);

    fetch::math::SizeType random_seed{};
    std::vector<uint64_t> buffer{};
    auto                  index = fetch::math::numeric_max<uint64_t>();
    map.ExpectKeyGetValue(RAND_SEED, random_seed);
    map.ExpectKeyGetValue(RAND_BUFFER, buffer);
    map.ExpectKeyGetValue(RAND_INDEX, index);
    sp.rand.Seed(random_seed);
    sp.rand.SetBuffer(buffer);
    sp.rand.SetIndex(index);

    map.ExpectKeyGetValue(SIZE_NOT_SET, sp.size_not_set_);
    map.ExpectKeyGetValue(CUR_TRAINING_PAIR_FIRST, sp.cur_training_pair_.first);
    map.ExpectKeyGetValue(CUR_TRAINING_PAIR_SECOND, sp.cur_training_pair_.second);
    map.ExpectKeyGetValue(RET_PAIR_FIRST, sp.ret_pair_.first);
    map.ExpectKeyGetValue(RET_PAIR_SECOND, sp.ret_pair_.second);
  }
};

}  // namespace serializers

}  // namespace fetch
