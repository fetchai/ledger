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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include <random>

namespace fetch {
namespace dmlf {
namespace colearn {

class RandomDouble
{
public:
  using Bytes      = byte_array::ByteArray;
  using CBytes     = byte_array::ConstByteArray;
  using Underlying = std::uniform_real_distribution<double>;
  using Twister    = std::mt19937;

  RandomDouble();
  virtual ~RandomDouble() = default;

  RandomDouble(RandomDouble const &other) = delete;
  RandomDouble &operator=(RandomDouble const &other)  = delete;
  bool          operator==(RandomDouble const &other) = delete;
  bool          operator<(RandomDouble const &other)  = delete;

  virtual double GetAgain() const;
  virtual double GetNew();
  virtual void   Seed(Bytes const &data);
  virtual void   Seed(CBytes const &data);

  virtual void Set(double forced_value);

protected:
private:
  double             cache_ = 0;
  std::random_device rd_;
  Twister            twister_;
  Underlying         underlying_;
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
