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

#include "dmlf/iupdate.hpp"

#include <vector>
#include <cstdint>
#include <chrono>

namespace fetch {
namespace dmlf {

template <typename T>
class Update : public IUpdate
{
public:
  using TensorType       = T;
  using VectorTensorType = std::vector<TensorType>;
  using TimeStampType    = IUpdate::TimeStampType; 
  
  explicit Update()
    : stamp_{CurrentTime()}
  {
  }
  explicit Update(VectorTensorType gradients)
    : gradients_{gradients}
    , stamp_{CurrentTime()}
  {
  }
  
  virtual byte_array::ByteArray serialise() override
  {
    return byte_array::ByteArray{};
  }
  virtual void deserialise(byte_array::ByteArray&) override
  {
  }
  virtual TimeStampType TimeStamp() const override 
  {
    return stamp_;
  }

  virtual ~Update()
  {
  }
protected:
private:
  Update(const Update &other) = delete;
  Update &operator=(const Update &other) = delete;
  bool operator==(const Update &other) = delete;
  bool operator<(const Update &other) = delete;
  
  TimeStampType CurrentTime() 
  {
    return static_cast<TimeStampType>
      (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
  }
  
  VectorTensorType gradients_;
  TimeStampType stamp_;
};

}  // namespace dmlf
}  // namespace fetch
