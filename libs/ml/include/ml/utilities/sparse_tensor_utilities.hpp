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

#include "math/tensor.hpp"

#include <vector>

namespace fetch {
namespace ml {
namespace utilities {

/**
 * Add update_rows from src Tensor to dst Tensor if number of required slices is lower than
 * threshold. If number of required slices is higher than threshold, function just inlineAdd src to
 * dst.
 * @tparam TensorType
 * @param src source tensor
 * @param dst destination tensor
 * @param update_rows set of rows to be updated
 * @param sparsity_threshold
 */
template <class TensorType>
void SparseAdd(TensorType const &src, TensorType &dst,
               std::unordered_set<fetch::math::SizeType> const &update_rows,
               fetch::math::SizeType                            sparsity_threshold = 4)
{
  using SizeType = fetch::math::SizeType;

  // Normal apply
  // if number_of_rows_to_update*sparsity_threshold_ > total_rows
  if (update_rows.empty() || (update_rows.size() * sparsity_threshold) > (dst.shape().at(1)))
  {
    dst.InlineAdd(src);
  }

  // Sparse apply
  // if number_of_rows_to_update*sparsity_threshold_ <= total_rows
  else
  {
    memory::Range range(0, std::size_t(dst.height()));

    for (SizeType update_index : update_rows)
    {
      auto dst_slice = dst.data().slice(dst.padded_height() * update_index, dst.padded_height());
      auto src_slice = src.data().slice(src.padded_height() * update_index, src.padded_height());

      // Parallel addition
      dst_slice.in_parallel().RangedApplyMultiple(
          range, [](auto const &a, auto const &b, auto &c) { c = b + a; }, dst_slice, src_slice);
    }
  }
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
