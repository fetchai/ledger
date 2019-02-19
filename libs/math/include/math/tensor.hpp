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
#include "math/free_functions/standard_functions/abs.hpp"

#include <iostream>
#include <memory>
#include <vector>

namespace fetch {
namespace math {

template <typename T>
class Tensor  // Using name Tensor to not clash with current NDArray
{
public:
  using Type                           = T;
  static const size_t DefaultAlignment = 8;  // Arbitrary picked

public:
  Tensor(std::vector<size_t>             shape   = std::vector<size_t>(),
         std::vector<size_t>             strides = std::vector<size_t>(),
         std::vector<size_t>             padding = std::vector<size_t>(),
         std::shared_ptr<std::vector<T>> storage = nullptr, size_t offset = 0)
    : shape_(std::move(shape))
    , padding_(std::move(padding))
    , strides_(std::move(strides))
    , storage_(std::move(storage))
    , offset_(offset)
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
        padding_        = std::vector<size_t>(shape_.size(), 0);
        padding_.back() = DefaultAlignment - ((strides_.back() * shape_.back()) % DefaultAlignment);
      }
      size_t dim = 1;
      for (size_t i(shape_.size()); i-- > 0;)
      {
        dim *= strides_[i];
        strides_[i] = dim;
        dim *= shape_[i];
        dim += padding_[i];
      }
      if (!storage_)
      {
        offset_ = 0;
        if (!shape_.empty())
        {
          storage_ = std::make_shared<std::vector<T>>(
              std::max(1ul, DimensionSize(0) * shape_[0] + padding_[0]));
        }
      }
    }
  }

  std::vector<size_t> const &shape()
      const  // TODO(private, 520) fix capitalisation (kepping it consistent with NDArray for now)
  {
    return shape_;
  }

  size_t DimensionSize(size_t dim) const
  {
    if (!shape_.empty() && dim < shape_.size())
    {
      return strides_[dim];
    }
    return 0;
  }

  size_t Capacity() const
  {
    return storage_ ? storage_->size() : 0;
  }

  // TODO(private, 520): fix capitalisation (kepping it consistent with NDArray for now)
  size_t size() const
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

  /**
   * Return the coordinates of the nth element in N dimensions
   * @param element     ordinal position of the element we want
   * @return            coordinate of said element in the tensor
   */
  std::vector<size_t> IndicesOfElement(size_t element) const
  {
    ASSERT(element < size());
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
    size_t index(offset_);
    for (size_t i(0); i < indices.size(); ++i)
    {
      ASSERT(indices[i] < shape_[i]);
      index += indices[i] * DimensionSize(i);
    }
    return index;
  }

  void Fill(T const &value)
  {
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = value;
    }
  }

  void Copy(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = o.At(i);
    }
  }

  /////////////////
  /// ACCESSORS ///
  /////////////////

  T &At(size_t i)
  {
    return (*storage_)[OffsetOfElement(IndicesOfElement(i))];
  }

  T const &At(size_t i) const
  {
    return (*storage_)[OffsetOfElement(IndicesOfElement(i))];
  }

  T const &operator()(std::vector<size_t> const &indices) const
  {
    return Get(indices);
  }

  T const &Get(std::vector<size_t> const &indices) const
  {
    return (*storage_)[OffsetOfElement(indices)];
  }

  T &Get(std::vector<size_t> const &indices)
  {
    return (*storage_)[OffsetOfElement(indices)];
  }

  ///////////////
  /// SETTERS ///
  ///////////////

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

  /*
   * return a slice of the tensor along the first dimension
   */
  Tensor<T> Slice(size_t i)
  {
    assert(shape_.size() > 1 && i < shape_[0]);
    Tensor<T> ret(std::vector<size_t>(std::next(shape_.begin()), shape_.end()),     /* shape */
                  std::vector<size_t>(std::next(strides_.begin()), strides_.end()), /* stride */
                  std::vector<size_t>(std::next(padding_.begin()), padding_.end()), /* padding */
                  storage_, offset_ + i * DimensionSize(0));
    ret.strides_ = std::vector<size_t>(std::next(strides_.begin()), strides_.end());
    ret.padding_ = std::vector<size_t>(std::next(padding_.begin()), padding_.end());
    return ret;
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
    ASSERT(o.size() == size());
    for (size_t i(0); i < size(); ++i)
    {
      T e1 = Get(IndicesOfElement(i));
      T e2 = o.Get(o.IndicesOfElement(i));

      T abs_e1 = e1;
      fetch::math::Abs(abs_e1);
      T abs_e2 = e2;
      fetch::math::Abs(abs_e2);
      T abs_diff = e1 - e2;
      fetch::math::Abs(abs_diff);
      T tolerance = std::max(absolute_tolerance, std::max(abs_e1, abs_e2) * relative_tolerance);
      if (abs_diff > tolerance)
      {
        std::cout << "AllClose[" << i << "] - " << e1 << " != " << e2 << std::endl;
        return false;
      }
    }
    return true;
  }

  Tensor<T> &InlineAdd(T const &o)
  {
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = At(i) + o;
    }
    return *this;
  }

  Tensor<T> &InlineAdd(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = At(i) + o.At(i);
    }
    return *this;
  }

  Tensor<T> &InlineSubtract(T const &o)
  {
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = At(i) - o;
    }
    return *this;
  }

  Tensor<T> &InlineSubtract(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = At(i) - o.At(i);
    }
    return *this;
  }

  Tensor<T> &InlineReverseSubtract(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = o.At(i) - At(i);
    }
    return *this;
  }

  Tensor<T> &InlineMultiply(T const &o)
  {
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = At(i) * o;
    }
    return *this;
  }

  Tensor<T> &InlineMultiply(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = At(i) * o.At(i);
    }
    return *this;
  }

  Tensor<T> &InlineDivide(T const &o)
  {
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = At(i) / o;
    }
    return *this;
  }

  Tensor<T> &InlineDivide(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (size_t i(0); i < size(); ++i)
    {
      At(i) = At(i) / o.At(i);
    }
    return *this;
  }

  T Sum() const
  {
    T sum(0);
    for (size_t i(0); i < size(); ++i)
    {
      sum = sum + At(i);
    }
    return sum;
  }

  Tensor<T> Transpose() const
  {
    assert(shape_.size() == 2);
    Tensor<T> ret(std::vector<size_t>({shape_[1], shape_[0]}), /* shape */
                  std::vector<size_t>(),                       /* stride */
                  std::vector<size_t>(),                       /* padding */
                  storage_, offset_);
    ret.strides_ = std::vector<size_t>(strides_.rbegin(), strides_.rend());
    ret.padding_ = std::vector<size_t>(padding_.rbegin(), padding_.rend());
    return ret;
  }

  std::string ToString() const
  {
    std::stringstream ss;
    ss << std::setprecision(5) << std::fixed << std::showpos;
    if (shape_.size() == 2)
    {
      for (size_t i(0); i < shape_[0]; ++i)
      {
        for (size_t j(0); j < shape_[1]; ++j)
        {
          ss << Get({i, j}) << "\t";
        }
        ss << "\n";
      }
    }
    return ss.str();
  }

private:
  std::vector<size_t>             shape_;
  std::vector<size_t>             padding_;
  std::vector<size_t>             strides_;
  std::shared_ptr<std::vector<T>> storage_;
  size_t                          offset_;
};
}  // namespace math
}  // namespace fetch
