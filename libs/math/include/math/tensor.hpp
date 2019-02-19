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
  using Type                             = T;
  using SizeType                         = std::uint64_t;
  static const SizeType DefaultAlignment = 8;  // Arbitrary picked

public:
  Tensor(std::vector<SizeType>           shape   = std::vector<SizeType>(),
         std::vector<SizeType>           strides = std::vector<SizeType>(),
         std::vector<SizeType>           padding = std::vector<SizeType>(),
         std::shared_ptr<std::vector<T>> storage = nullptr, SizeType offset = 0)
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

  Tensor(SizeType size)
    : shape_({size})
  {
    Init(strides_, padding_);
  }

  void Init(std::vector<SizeType> const &strides = std::vector<SizeType>(),
            std::vector<SizeType> const &padding = std::vector<SizeType>())
  {
    if (!shape_.empty())
    {
      if (strides.empty())
      {
        strides_ = std::vector<SizeType>(shape_.size(), 1);
      }
      if (padding.empty())
      {
        padding_        = std::vector<SizeType>(shape_.size(), 0);
        padding_.back() = DefaultAlignment - ((strides_.back() * shape_.back()) % DefaultAlignment);
      }
      SizeType dim = 1;
      for (SizeType i(shape_.size()); i-- > 0;)
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
              std::max(SizeType(1), DimensionSize(0) * shape_[0] + padding_[0]));
        }
      }
    }
  }

  std::vector<SizeType> const &shape()
      const  // TODO(private, 520) fix capitalisation (kepping it consistent with NDArray for now)
  {
    return shape_;
  }

  SizeType DimensionSize(SizeType dim) const
  {
    if (!shape_.empty() && dim < shape_.size())
    {
      return strides_[dim];
    }
    return 0;
  }

  SizeType Capacity() const
  {
    return storage_ ? storage_->size() : 0;
  }

  // TODO(private, 520): fix capitalisation (kepping it consistent with NDArray for now)
  SizeType size() const
  {
    if (shape_.empty())
    {
      return 0;
    }
    SizeType n(1);
    for (SizeType d : shape_)
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
  std::vector<SizeType> IndicesOfElement(SizeType element) const
  {
    ASSERT(element < size());
    std::vector<SizeType> results(shape_.size());
    results.back() = element;
    for (SizeType i(shape_.size() - 1); i > 0; --i)
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
  SizeType OffsetOfElement(std::vector<SizeType> const &indices) const
  {
    SizeType index(offset_);
    for (SizeType i(0); i < indices.size(); ++i)
    {
      ASSERT(indices[i] < shape_[i]);
      index += indices[i] * DimensionSize(i);
    }
    return index;
  }

  void Fill(T const &value)
  {
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = value;
    }
  }

  void Copy(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = o.At(i);
    }
  }

  /////////////////
  /// ACCESSORS ///
  /////////////////

  T &At(SizeType i)
  {
    return (*storage_)[OffsetOfElement(IndicesOfElement(i))];
  }

  T const &At(SizeType i) const
  {
    return (*storage_)[OffsetOfElement(IndicesOfElement(i))];
  }

  T const &operator()(std::vector<SizeType> const &indices) const
  {
    return Get(indices);
  }

  T const &Get(std::vector<SizeType> const &indices) const
  {
    return (*storage_)[OffsetOfElement(indices)];
  }

  T &Get(std::vector<SizeType> const &indices)
  {
    return (*storage_)[OffsetOfElement(indices)];
  }

  ///////////////
  /// SETTERS ///
  ///////////////

  void Set(std::vector<SizeType> const &indices, T value)
  {
    (*storage_)[OffsetOfElement(indices)] = value;
  }

  void Set(SizeType i, T value)
  {
    (*storage_)[OffsetOfElement(IndicesOfElement(i))] = value;
  }

  T &operator[](SizeType i)
  {
    return At(i);
  }

  /*
   * return a slice of the tensor along the first dimension
   */
  Tensor<T> Slice(SizeType i)
  {
    assert(shape_.size() > 1 && i < shape_[0]);
    Tensor<T> ret(std::vector<SizeType>(std::next(shape_.begin()), shape_.end()),     /* shape */
                  std::vector<SizeType>(std::next(strides_.begin()), strides_.end()), /* stride */
                  std::vector<SizeType>(std::next(padding_.begin()), padding_.end()), /* padding */
                  storage_, offset_ + i * DimensionSize(0));
    ret.strides_ = std::vector<SizeType>(std::next(strides_.begin()), strides_.end());
    ret.padding_ = std::vector<SizeType>(std::next(padding_.begin()), padding_.end());
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
    for (SizeType i(0); i < size(); ++i)
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
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = At(i) + o;
    }
    return *this;
  }

  Tensor<T> &InlineAdd(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = At(i) + o.At(i);
    }
    return *this;
  }

  Tensor<T> &InlineSubtract(T const &o)
  {
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = At(i) - o;
    }
    return *this;
  }

  Tensor<T> &InlineSubtract(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = At(i) - o.At(i);
    }
    return *this;
  }

  Tensor<T> &InlineReverseSubtract(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = o.At(i) - At(i);
    }
    return *this;
  }

  Tensor<T> &InlineMultiply(T const &o)
  {
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = At(i) * o;
    }
    return *this;
  }

  Tensor<T> &InlineMultiply(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = At(i) * o.At(i);
    }
    return *this;
  }

  Tensor<T> &InlineDivide(T const &o)
  {
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = At(i) / o;
    }
    return *this;
  }

  Tensor<T> &InlineDivide(Tensor<T> const &o)
  {
    assert(size() == o.size());
    for (SizeType i(0); i < size(); ++i)
    {
      At(i) = At(i) / o.At(i);
    }
    return *this;
  }

  T Sum() const
  {
    T sum(0);
    for (SizeType i(0); i < size(); ++i)
    {
      sum = sum + At(i);
    }
    return sum;
  }

  Tensor<T> Transpose() const
  {
    assert(shape_.size() == 2);
    Tensor<T> ret(std::vector<SizeType>({shape_[1], shape_[0]}), /* shape */
                  std::vector<SizeType>(),                       /* stride */
                  std::vector<SizeType>(),                       /* padding */
                  storage_, offset_);
    ret.strides_ = std::vector<SizeType>(strides_.rbegin(), strides_.rend());
    ret.padding_ = std::vector<SizeType>(padding_.rbegin(), padding_.rend());
    return ret;
  }

  std::string ToString() const
  {
    std::stringstream ss;
    ss << std::setprecision(5) << std::fixed << std::showpos;
    if (shape_.size() == 2)
    {
      for (SizeType i(0); i < shape_[0]; ++i)
      {
        for (SizeType j(0); j < shape_[1]; ++j)
        {
          ss << Get({i, j}) << "\t";
        }
        ss << "\n";
      }
    }
    return ss.str();
  }

private:
  std::vector<SizeType>           shape_;
  std::vector<SizeType>           padding_;
  std::vector<SizeType>           strides_;
  std::shared_ptr<std::vector<T>> storage_;
  SizeType                        offset_;
};
}  // namespace math
}  // namespace fetch
