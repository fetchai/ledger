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

#include "core/bloom_filter.hpp"
#include "core/bloom_filter_interface.hpp"

#include <memory>

namespace fetch {

std::unique_ptr<BloomFilterInterface> BloomFilterInterface::create(Type type)
{
  switch (type)
  {
  case Type::BASIC:
    return std::make_unique<fetch::BasicBloomFilter>();
  case Type::DUMMY:
    return std::make_unique<fetch::DummyBloomFilter>();
  }

  return nullptr;
}

}  // namespace fetch
