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
               fetch::math::SizeType                            sparsity_threshold = 2);

template <class TensorType>
void SparseAdd(TensorType const &src, TensorType &dst,
               std::vector<fetch::math::SizeType> const &update_rows);

template <class TensorType>
TensorType ToSparse(TensorType const &                               src,
                    std::unordered_set<fetch::math::SizeType> const &update_rows);

template <class TensorType>
TensorType FromSparse(TensorType const &                               src,
                      std::unordered_set<fetch::math::SizeType> const &update_rows,
                      fetch::math::SizeType                            output_rows);

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
