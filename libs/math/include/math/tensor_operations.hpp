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

#include "core/assert.hpp"
#include "math/tensor.hpp"
#include <memory>
#include <vector>

namespace fetch {
namespace math {

/*
 * Concatenate tensor by creating a new leading dimention
 * Example [2, 5, 5] + [2, 5, 5] + [2, 5, 5] = [3, 2, 5, 5]
 * Returns newly allocated memory
 */
template <typename T>
fetch::math::Tensor<T> ConcatenateTensors(std::vector<fetch::math::Tensor<T>> const &tensors)
{
  std::vector<typename fetch::math::Tensor<T>::SizeType> retSize;
  retSize.push_back(tensors.size());
  retSize.insert(retSize.end(), tensors.front().shape().begin(), tensors.front().shape().end());
  fetch::math::Tensor<T> ret(retSize);
  for (typename fetch::math::Tensor<T>::SizeType i(0); i < tensors.size(); ++i)
  {
    ret.Slice(i).Copy(tensors[i]);
  }
  return ret;
}

template <typename SizeType>
std::vector<SizeType> _get_cumsum(std::vector<SizeType> inp)
{
  std::vector<SizeType> result(inp.size());
  result[0] = inp[0];
  for (SizeType i = 1; i < result.size(); i++)
  {
    result[i] = result[i - 1] + inp[i];
  }
  return result;
}

template <typename SizeType>
SizeType _get_array_number(SizeType pos, std::vector<SizeType> array_sizes_cumsum)
{
  for (SizeType i = 0; i < array_sizes_cumsum.size(); i++)
  {
    if (pos < array_sizes_cumsum[i])
    {
      return i;
    };
  }
  return SizeType{0};
}

template <typename T>
void _assert_concat_tensor_shapes(std::vector<T> tensors, typename T::SizeType concat_axis)
{
  std::vector<typename T::SizeType> n_dims(tensors.size());

  for (typename T::SizeType i = 0; i < tensors.size(); i++)
  {
    n_dims[i] = tensors[i].shape().size();
  }

  ASSERT(std::equal(n_dims.begin() + 1, n_dims.end(), n_dims.begin()));

  for (typename T::SizeType d = 0; d < tensors[0].shape().size(); d++)
  {
    if (d != concat_axis)
    {
      std::vector<typename T::SizeType> dim_sizes(tensors.size());
      for (typename T::SizeType i = 0; i < tensors.size(); i++)
      {
        dim_sizes[i] = tensors[i].shape()[d];
      }
      ASSERT(std::equal(dim_sizes.begin() + 1, dim_sizes.end(), dim_sizes.begin()));
    }
  }
}

template <typename T>
std::vector<typename T::SizeType> _infer_shape_of_concat_tensors(std::vector<T>       tensors,
                                                                 typename T::SizeType axis)
{
  // asserts!

  std::vector<typename T::SizeType> final_shape(tensors[0].shape().size());

  for (typename T::SizeType i = 0; i < tensors[0].shape().size(); i++)
  {
    if (i != axis)
    {
      final_shape[i] = tensors[0].shape()[i];
    }
    else
    {
      for (typename T::SizeType j = 0; j < tensors.size(); j++)
      {
        final_shape[i] += tensors[j].shape()[i];
      }
    }
  }

  return final_shape;
}

template <typename T>
std::vector<typename T::SizeType> _get_dims_along_ax_cumsummed(std::vector<T>       tensors,
                                                               typename T::SizeType axis)
{
  std::vector<typename T::SizeType> dim_along_concat_ax(tensors.size());

  for (typename T::SizeType i = 0; i < tensors.size(); i++)
  {
    dim_along_concat_ax[i] = tensors[i].shape()[axis];
  }

  std::vector<typename T::SizeType> dim_along_concat_ax_csummed{
      _get_cumsum<typename T::SizeType>(dim_along_concat_ax)};

  return dim_along_concat_ax_csummed;
}

template <typename T>
void _concatenate_assign_values(T &res_tensor, std::vector<T> tensors,
                                typename T::SizeType               concat_axis,
                                std::vector<typename T::SizeType> &counter)
{
  std::vector<typename T::SizeType> tensors_concat_dim_info{
      _get_dims_along_ax_cumsummed<T>(tensors, concat_axis)},
      counter_copy(counter);
  typename T::SizeType pos_in_n_arr,
      n_arr{_get_array_number<typename T::SizeType>(counter[concat_axis], tensors_concat_dim_info)};

  for (typename T::SizeType i = 0; i < counter.size(); ++i)
  {
    if (i == concat_axis)
    {
      if (counter[i] >= tensors_concat_dim_info[0])
      {
        pos_in_n_arr = counter[i] - tensors_concat_dim_info[n_arr - 1];
      }
      else
      {
        pos_in_n_arr = counter[i];
      }
      counter_copy[concat_axis] = pos_in_n_arr;
    }
  }
  res_tensor.At(counter) = tensors[n_arr].At(counter_copy);
}

template <typename T>
void _concatenate_recursive_dimension_lookup(T &res_tensor, std::vector<T> tensors,
                                             typename T::SizeType              concat_axis,
                                             std::vector<typename T::SizeType> counter,
                                             typename T::SizeType              no_dim)
{
  if (no_dim != res_tensor.shape().size())
  {
    for (counter[no_dim] = 0; counter[no_dim] < res_tensor.shape()[no_dim]; ++counter[no_dim])
    {
      _concatenate_recursive_dimension_lookup(res_tensor, tensors, concat_axis, counter,
                                              (1 + no_dim));
    }
  }
  else
  {
    _concatenate_assign_values(res_tensor, tensors, concat_axis, counter);
  }
}

template <typename T>
T concatenate(std::vector<T> tensors, typename T::SizeType concat_axis)
{
  _assert_concat_tensor_shapes(tensors, concat_axis);
  std::vector<typename T::SizeType> res_tensor_shape{
      _infer_shape_of_concat_tensors(tensors, concat_axis)};
  T res_tensor(res_tensor_shape);

  _concatenate_recursive_dimension_lookup(
      res_tensor, tensors, concat_axis,
      std::vector<typename T::SizeType>(res_tensor_shape.size(), 0), typename T::SizeType{0});

  return res_tensor;
}

}  // namespace math
}  // namespace fetch
