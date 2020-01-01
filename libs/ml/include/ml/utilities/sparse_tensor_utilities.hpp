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

#include "math/tensor/tensor.hpp"

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
               fetch::math::SizeType                            sparsity_threshold = 2)
{
  using SizeType = fetch::math::SizeType;

  // Normal apply
  // if number_of_rows_to_update * sparsity_threshold_ > total_rows
  if ((update_rows.empty() || (update_rows.size() * sparsity_threshold) > (dst.shape().at(1))) &&
      (update_rows.size() != src.shape().at(0)))
  {
    dst.InlineAdd(src);
  }

  // Sparse apply
  // if number_of_rows_to_update * sparsity_threshold_ <= total_rows
  else
  {
    memory::Range range(0, std::size_t(dst.height()));

    SizeType cnt = 0;
    SizeType src_index;
    SizeType dst_index;
    for (SizeType update_index : update_rows)
    {

      // Sparse update full tensor to full tensor
      if (update_rows.size() != src.shape().at(0))
      {
        src_index = update_index;
        dst_index = update_index;
      }
      // Sparse update src sparse tensor to normal dst tensor
      else
      {
        src_index = cnt;
        dst_index = update_index;
      }

      auto dst_slice = dst.data().slice(dst.padded_height() * dst_index, dst.padded_height());
      auto src_slice = src.data().slice(src.padded_height() * src_index, src.padded_height());

      // Parallel addition
      dst_slice.in_parallel().RangedApplyMultiple(
          range, [](auto const &a, auto const &b, auto &c) { c = b + a; }, dst_slice, src_slice);
      cnt++;
    }
  }
}

template <class TensorType>
void SparseAdd(TensorType const &src, TensorType &dst,
               std::vector<fetch::math::SizeType> const &update_rows)
{
  using SizeType = fetch::math::SizeType;

  memory::Range range(0, std::size_t(dst.height()));

  SizeType src_index = 0;
  for (SizeType dst_index : update_rows)
  {
    if (dst_index == fetch::math::numeric_max<SizeType>())
    {
      // Unknown word to be skipped
      src_index++;
      continue;
    }
    auto dst_slice = dst.data().slice(dst.padded_height() * dst_index, dst.padded_height());
    auto src_slice = src.data().slice(src.padded_height() * src_index, src.padded_height());

    // Parallel addition
    dst_slice.in_parallel().RangedApplyMultiple(
        range, [](auto const &a, auto const &b, auto &c) { c = b + a; }, dst_slice, src_slice);
    src_index++;
  }
}

template <class TensorType>
TensorType ToSparse(TensorType const &                               src,
                    std::unordered_set<fetch::math::SizeType> const &update_rows)
{
  using SizeType = fetch::math::SizeType;

  TensorType dst({src.shape().at(0), update_rows.size()});

  memory::Range range(0, std::size_t(dst.height()));

  SizeType dst_index = 0;
  for (SizeType src_index : update_rows)
  {
    auto dst_slice = dst.data().slice(dst.padded_height() * dst_index, dst.padded_height());
    auto src_slice = src.data().slice(src.padded_height() * src_index, src.padded_height());

    // Parallel addition
    dst_slice.in_parallel().RangedApplyMultiple(
        range, [](auto const &a, auto const &b, auto &c) { c = b + a; }, dst_slice, src_slice);
    dst_index++;
  }

  return std::move(dst);
}

template <class TensorType>
TensorType FromSparse(TensorType const &                               src,
                      std::unordered_set<fetch::math::SizeType> const &update_rows,
                      fetch::math::SizeType                            output_rows)
{
  using SizeType = fetch::math::SizeType;

  TensorType dst({src.shape().at(0), output_rows});

  SizeType src_index = 0;
  for (SizeType dst_index : update_rows)
  {
    auto dst_view = dst.View(dst_index);
    auto src_view = src.View(src_index);
    dst_view.Assign(src_view);

    src_index++;
  }

  return std::move(dst);
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
