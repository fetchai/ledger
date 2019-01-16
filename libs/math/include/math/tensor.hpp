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
#include <iostream>
#include <memory>
#include <vector>

namespace fetch {
namespace math {

template <typename T>
class Tensor  // Using name Tensor to not clash with current NDArray
{
public:
  using Type = T;

public:
  Tensor(std::vector<size_t> shape   = std::vector<size_t>(),
         std::vector<size_t> strides = std::vector<size_t>(),
         std::vector<size_t> padding = std::vector<size_t>())
    : shape_(std::move(shape))
    , padding_(std::move(padding))
    , strides_(std::move(strides))
  {
    ASSERT(padding.empty() || padding.size() == shape.size());
    ASSERT(strides.empty() || strides.size() == shape.size());
    Init(strides_, padding_);
  }

  Tensor(size_t size)
    : shape_({size})
  {
    Init(strides_, padding_);
  }

  void Init(std::vector<size_t> const &strides = std::vector<size_t>(),
            std::vector<size_t> const &padding = std::vector<size_t>())
  {
    if (!shape_.empty())
    {
      if (strides.empty())
      {
        strides_ = std::vector<size_t>(shape_.size(), 1);
      }
      if (padding.empty())
      {
        padding_ = std::vector<size_t>(shape_.size(), 0);
        padding_.back() =
            8 - ((strides_.back() * shape_.back()) % 8);  // Let's align everything on 8
      }
      storage_ = std::make_shared<std::vector<T>>(Capacity());
    }
  }

  std::vector<size_t> const &shape()
      const  // TODO(private, 520) fix capitalisation (kepping it consistent with NDArray for now)
  {
    return shape_;
  }

  size_t NumberOfElements() const
  {
    if (shape_.empty())
    {
      return 0;
    }
    size_t n(1);
    for (size_t d : shape_)
    {
      n *= d;
    }
    return n;
  }

  size_t DimensionSize(size_t dim) const
  {
    if (!shape_.empty() && dim >= 0 && dim < shape_.size())
    {
      return DimensionSizeImplementation(dim);
    }
    return 0;
  }

  size_t Capacity() const
  {
    if (shape_.empty())
    {
      return 0;
    }
    return std::max(1ul, DimensionSize(0) * shape_[0] + padding_[0]);
  }

  size_t size()
      const  // TODO(private, 520): fix capitalisation (kepping it consistent with NDArray for now)
  {
    return NumberOfElements();
  }

  /**
   * Return the coordinates of the nth element in N dimensions
   * @param element     ordinal position of the element we want
   * @return            coordinate of said element in the tensor
   */
  std::vector<size_t> IndicesOfElement(size_t element) const
  {
    ASSERT(element < NumberOfElements());
    std::vector<size_t> results(shape_.size());
    results.back() = element;
    for (size_t i(shape_.size() - 1); i > 0; --i)
    {
      results[i - 1] = results[i] / shape_[i];
      results[i] %= shape_[i];
    }
    return results;
  }

  /**
   * Return the offset of element at specified coordinates in the low level memory array
   * @param indices     coordinate of requested element in the tensor
   * @return            offset in low level memory array
   */
  size_t OffsetOfElement(std::vector<size_t> const &indices) const
  {
    size_t index(0);
    for (size_t i(0); i < indices.size(); ++i)
    {
      ASSERT(indices[i] < shape_[i]);
      index += indices[i] * DimensionSize(i);
    }
    return index;
  }

  void Fill(T const &value)
  {
    for (size_t i(0); i < NumberOfElements(); ++i)
    {
      this->operator[](i) = value;
    }
  }

  T &At(size_t i)
  {
    return (*storage_)[OffsetOfElement(IndicesOfElement(i))];
  }

  T Get(std::vector<size_t> const &indices) const
  {
    return (*storage_)[OffsetOfElement(indices)];
  }

  void Set(std::vector<size_t> const &indices, T value)
  {
    (*storage_)[OffsetOfElement(indices)] = value;
  }

  void Set(size_t i, T value)
  {
    (*storage_)[OffsetOfElement(IndicesOfElement(i))] = value;
  }

  T &operator[](size_t i)
  {
    return At(i);
  }

  std::shared_ptr<const std::vector<T>> Storage() const
  {
    return storage_;
  }

  bool AllClose(Tensor<T> const &o, T const &relative_tolerance = T(1e-5),
                T const &absolute_tolerance = T(1e-8))
  {
    // Only enforcing number of elements
    // we allow for different shapes as long as element are in same order
    ASSERT(o.NumberOfElements() == NumberOfElements());
    for (size_t i(0); i < NumberOfElements(); ++i)
    {
      T e1        = Get(IndicesOfElement(i));
      T e2        = o.Get(o.IndicesOfElement(i));
      T tolerance = std::max(absolute_tolerance, std::max(abs(e1), abs(e2)) * relative_tolerance);
      if (abs(e1 - e2) > tolerance)
      {
        std::cout << "AllClose[" << i << "] - " << e1 << " != " << e2 << std::endl;
        return false;
      }
    }
    return true;
  }

private:
  size_t DimensionSizeImplementation(size_t dim) const
  {
    if (dim == shape_.size() - 1)
    {
      return strides_[dim];
    }
    return (DimensionSizeImplementation(dim + 1) * shape_[dim + 1] + padding_[dim + 1]) *
           strides_[dim];
  }

private:
  std::shared_ptr<std::vector<T>> storage_;
  std::vector<size_t>             shape_;
  std::vector<size_t>             padding_;
  std::vector<size_t>             strides_;
};
}  // namespace math
}  // namespace fetch
