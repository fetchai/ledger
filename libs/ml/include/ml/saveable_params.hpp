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

#include "math/base_types.hpp"
#include "ml/ops/ops.hpp"
#include "ml/regularisers/regularisation.hpp"

namespace fetch {
namespace ml {

/* Generic container for all the saveable params of an op.
 * Some ops will have to declare subclasses (substructs?) of this.
 */
struct SaveableParams
{
  // needs to have at least one virtual funciton to be a polymorphic class
  virtual ~SaveableParams() = default;
  std::string DESCRIPTOR;  // description of op
};

template <class ArrayType>
struct WeightsSaveableParams : public SaveableParams
{
  std::shared_ptr<ArrayType>             output;
  fetch::ml::details::RegularisationType regularisation_type{};
  typename ArrayType::Type               regularisation_rate;
};

struct DropoutSaveableParams : public SaveableParams
{
  fetch::math::SizeType random_seed;
  double                probability;
};

struct LeakyReluSaveableParams : public SaveableParams
{
  double a;
};

struct SoftmaxSaveableParams : public SaveableParams
{
  fetch::math::SizeType axis;
};

struct TransposeSaveableParams : public SaveableParams
{
  std::vector<fetch::math::SizeType> transpose_vector;
};

template <class T>
struct GraphSaveableParams
{
  // unique node name to list of inputs
  std::vector<std::pair<std::string, std::vector<std::string>>>    connections;
  std::unordered_map<std::string, std::shared_ptr<SaveableParams>> nodes;
};
}  // namespace ml
}  // namespace fetch
