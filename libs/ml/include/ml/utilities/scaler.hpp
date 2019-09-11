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

namespace fetch {
namespace ml {
namespace utilities {

template <typename TensorType>
class Scaler
{
public:
  Scaler()          = default;
  virtual ~Scaler() = default;

  virtual void SetScale(TensorType const &reference_tensor)                           = 0;
  virtual void Normalise(TensorType const &input_tensor, TensorType &output_tensor)   = 0;
  virtual void DeNormalise(TensorType const &input_tensor, TensorType &output_tensor) = 0;
};

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
