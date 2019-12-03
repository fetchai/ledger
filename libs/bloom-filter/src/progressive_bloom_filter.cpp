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

#include "bloom_filter/progressive_bloom_filter.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <cstddef>

namespace fetch {

ProgressiveBloomFilter::ProgressiveBloomFilter(uint64_t const overlap)
  : overlap_{overlap}
{}

std::pair<bool, std::size_t> ProgressiveBloomFilter::Match(
    fetch::byte_array::ConstByteArray const &element, std::size_t element_index) const
{
  if (!IsInCurrentRange(element_index))
  {
    return {false, 0u};
  }

  return filter1_->Match(element);
}

void ProgressiveBloomFilter::Add(fetch::byte_array::ConstByteArray const &element,
                                 std::size_t element_index, std::size_t current_head_index)
{
  // Advance filter if necessary
  if (!IsInCurrentRange(current_head_index))
  {
    current_min_index_ += overlap_;
    filter1_->Reset();
    std::swap(filter1_, filter2_);
  }

  if (!IsInCurrentRange(element_index))
  {
    return;
  }

  filter1_->Add(element);

  if (current_min_index_ + overlap_ <= element_index)
  {
    filter2_->Add(element);
  }
}

bool ProgressiveBloomFilter::IsInCurrentRange(std::size_t const index) const
{
  return current_min_index_ <= index && index < current_min_index_ + 2 * overlap_;
}

void ProgressiveBloomFilter::Reset()
{
  filter1_->Reset();
  filter2_->Reset();
  current_min_index_ = 0u;
}

}  // namespace fetch
