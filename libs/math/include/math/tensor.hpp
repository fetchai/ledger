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
#include "math/standard_functions/abs.hpp"
#include "tensor_iterator.hpp"

#include "core/random/lcg.hpp"

#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

namespace fetch {
namespace math {

template <typename T>
class Tensor
{
public:
  using Type                             = T;
  using SizeType                         = std::uint64_t;
  using SelfType                         = Tensor<T>;
  static const SizeType DefaultAlignment = 8;  // Arbitrary picked

public:
  Tensor(std::vector<SizeType>           shape   = std::vector<SizeType>(),
         std::vector<SizeType>           strides = std::vector<SizeType>(),
         std::vector<SizeType>           padding = std::vector<SizeType>(),
         std::shared_ptr<std::vector<T>> storage = nullptr, SizeType offset = 0)
    : shape_(std::move(shape))
    , padding_(std::move(padding))
    , input_strides_(std::move(strides))
    , storage_(std::move(storage))
    , offset_(offset)
  {
    ASSERT(padding.empty() || padding.size() == shape.size());
    ASSERT(strides.empty() || strides.size() == shape.size());
    Init(input_strides_, padding_);
  }

  Tensor(SizeType size)
    : shape_({size})
  {
    Init(strides_, padding_);
  }

  Tensor(Tensor const &t)     = default;
  Tensor(Tensor &&t) noexcept = default;
  Tensor &operator=(Tensor const &other) = default;
  Tensor &operator=(Tensor &&) = default;

  /**
   * Initialises default values for stride padding etc.
   * @param strides
   * @param padding
   */
  void Init(std::vector<SizeType> const &strides = std::vector<SizeType>(),
            std::vector<SizeType> const &padding = std::vector<SizeType>())
  {
    if (!shape_.empty())
    {
      if (strides.empty())
      {
        strides_ = std::vector<SizeType>(shape_.size(), 1);
      }
      else
      {
        strides_ = strides;
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

  /**
   * Returns a deep copy of this tensor
   * @return
   */
  SelfType Clone() const
  {
    SelfType copy;

    copy.shape_   = this->shape_;
    copy.padding_ = this->padding_;
    copy.strides_ = this->strides_;
    copy.offset_  = this->offset_;

    if (storage_)
    {
      copy.storage_ = std::make_shared<std::vector<T>>(*storage_);
    }
    return copy;
  }

  /**
   * Copy data from another tensor into this one
   * assumes relevant stride/offset etc. are still applicable
   * @param other
   * @return
   */
  void Copy(SelfType const &other)
  {
    assert(other.size() == this->size());

    for (std::size_t j = 0; j < this->size(); ++j)
    {
      this->At(j) = other.At(j);
    }
  }

  // TODO(private, 520) fix capitalisation (kepping it consistent with NDArray for now)
  std::vector<SizeType> const &shape() const
  {
    return shape_;
  }

  std::vector<SizeType> const &Strides() const
  {
    return input_strides_;
  }

  std::vector<SizeType> const &Padding() const
  {
    return padding_;
  }

  SizeType Offset() const
  {
    return offset_;
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

  /////////////////
  /// Iterators ///
  /////////////////

  TensorIterator<T, SizeType> begin() const  // Need to stay lowercase for range basedloops
  {
    return TensorIterator<T, SizeType>(shape_, strides_, padding_,
                                       std::vector<SizeType>(shape_.size()), storage_, offset_);
  }

  TensorIterator<T, SizeType> end() const  // Need to stay lowercase for range basedloops
  {
    std::vector<SizeType> endCoordinate(shape_.size());
    endCoordinate[0] = shape_[0];
    return TensorIterator<T, SizeType>(shape_, strides_, padding_, endCoordinate, storage_,
                                       offset_);
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
    return At(indices);
  }

  T const &At(std::vector<SizeType> const &indices) const
  {
    return (*storage_)[OffsetOfElement(indices)];
  }

  T &At(std::vector<SizeType> const &indices)
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

  T const &operator[](SizeType i) const
  {
    return At(i);
  }

  /*
   * return a slice of the tensor along the first dimension
   */
  Tensor<T> Slice(SizeType i) const
  {
    assert(shape_.size() > 1 && i < shape_[0]);
    Tensor<T> ret(std::vector<SizeType>(std::next(shape_.begin()), shape_.end()),     /* shape */
                  std::vector<SizeType>(std::next(strides_.begin()), strides_.end()), /* stride */
                  std::vector<SizeType>(std::next(padding_.begin()), padding_.end()), /* padding */
                  storage_, offset_ + (i * DimensionSize(0)));
    ret.strides_ = std::vector<SizeType>(std::next(strides_.begin()), strides_.end());
    ret.padding_ = std::vector<SizeType>(std::next(padding_.begin()), padding_.end());
    return ret;
  }

  /*
   * Add a dummy leading dimension
   * Ex: [4, 5, 6].Unsqueeze() -> [1, 4, 5, 6]
   */
  Tensor<T> &Unsqueeze()
  {
    shape_.insert(shape_.begin(), 1);
    strides_.insert(strides_.begin(), strides_.front() * shape_[1]);
    padding_.insert(padding_.begin(), 0);
    return *this;
  }

  /*
   * Inverse of unsqueze : Collapse a empty leading dimension
   */
  Tensor<T> Squeeze()
  {
    if (shape_.front() == 1)
    {
      shape_.erase(shape_.begin());
      strides_.erase(strides_.begin());
      padding_.erase(padding_.begin());
    }
    else
    {
      throw std::runtime_error("Can't squeeze tensor with leading dimension of size " +
                               std::to_string(shape_[0]));
    }
    return *this;
  }

  std::shared_ptr<const std::vector<T>> Storage() const
  {
    return storage_;
  }

  bool AllClose(Tensor<T> const &o, T const &relative_tolerance = T(1e-5),
                T const &absolute_tolerance = T(1e-8)) const
  {
    // Only enforcing number of elements
    // we allow for different shapes as long as element are in same order
    ASSERT(o.size() == size());
    for (SizeType i(0); i < size(); ++i)
    {
      T e1 = At(IndicesOfElement(i));
      T e2 = o.At(o.IndicesOfElement(i));

      T abs_e1 = e1;
      fetch::math::Abs(abs_e1, abs_e1);
      T abs_e2 = e2;
      fetch::math::Abs(abs_e2, abs_e2);
      T abs_diff = e1 - e2;
      fetch::math::Abs(abs_diff, abs_diff);
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
    for (T &e : *this)
    {
      e += o;
    }
    return *this;
  }

  Tensor<T> &InlineAdd(Tensor<T> const &o)
  {
    assert(size() == o.size());
    auto it1 = this->begin();
    auto end = this->end();
    auto it2 = o.begin();

    while (it1 != end)
    {
      *it1 += *it2;
      ++it1;
      ++it2;
    }
    return *this;
  }

  Tensor<T> &InlineSubtract(T const &o)
  {
    for (T &e : *this)
    {
      e -= o;
    }
    return *this;
  }

  Tensor<T> &InlineSubtract(Tensor<T> const &o)
  {
    assert(size() == o.size());
    auto it1 = this->begin();
    auto end = this->end();
    auto it2 = o.begin();

    while (it1 != end)
    {
      *it1 -= *it2;
      ++it1;
      ++it2;
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
    for (T &e : *this)
    {
      e *= o;
    }
    return *this;
  }

  Tensor<T> &InlineMultiply(Tensor<T> const &o)
  {
    assert(size() == o.size());
    auto it1 = this->begin();
    auto end = this->end();
    auto it2 = o.begin();

    while (it1 != end)
    {
      *it1 *= *it2;
      ++it1;
      ++it2;
    }
    return *this;
  }

  Tensor<T> &InlineDivide(T const &o)
  {
    for (T &e : *this)
    {
      e /= o;
    }
    return *this;
  }

  Tensor<T> &InlineDivide(Tensor<T> const &o)
  {
    assert(size() == o.size());
    auto it1 = this->begin();
    auto end = this->end();
    auto it2 = o.begin();

    while (it1 != end)
    {
      *it1 /= *it2;
      ++it1;
      ++it2;
    }
    return *this;
  }

  T Sum() const
  {
    return std::accumulate(begin(), end(), T(0));
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

  /**
   * randomly reassigns the data within the tensor - expensive method since it requires data copy
   */
  void Shuffle()
  {
    std::default_random_engine rng{};
    std::vector<SizeType>      idxs(size());
    std::iota(std::begin(idxs), std::end(idxs), 0);
    std::shuffle(idxs.begin(), idxs.end(), rng);

    // instantiate new tensor with copy of data
    Tensor<Type> tmp = this->Clone();

    // copy data back according to shuffle
    for (std::size_t j = 0; j < tmp.size(); ++j)
    {
      this->Set(j, tmp.At(idxs[j]));
    }
  }

  std::string ToString() const
  {
    std::stringstream ss;
    ss << std::setprecision(5) << std::fixed << std::showpos;
    if (shape_.size() == 1)
    {
      for (SizeType i(0); i < shape_[0]; ++i)
      {
        ss << At(i) << "\t";
      }
    }
    if (shape_.size() == 2)
    {
      for (SizeType i(0); i < shape_[0]; ++i)
      {
        for (SizeType j(0); j < shape_[1]; ++j)
        {
          ss << At({i, j}) << "\t";
        }
        ss << "\n";
      }
    }
    return ss.str();
  }

  //////////////////////
  /// equality check ///
  //////////////////////

  /**
   * equality operator for tensors. checks size, shape, and data.
   * Fast when tensors not equal, slow otherwise
   * @param other
   * @return
   */
  bool operator==(Tensor const &other) const
  {
    bool ret = false;
    if ((this->size() == other.size()) && (this->shape_ == other.shape()))
    {
      ret = this->AllClose(other);
    }
    return ret;
  }

  bool operator!=(Tensor const &other) const
  {
    return !(*this == other);
  }

  static SelfType Concat(std::vector<SelfType> tensors, SizeType axis)
  {
    assert(ConcatTensorShapesValid(tensors, axis));
    return ConcatImplementation(tensors, axis);
  }

private:
  std::vector<SizeType>           shape_;
  std::vector<SizeType>           padding_;
  std::vector<SizeType>           strides_;
  std::vector<SizeType>           input_strides_;
  std::shared_ptr<std::vector<T>> storage_;
  SizeType                        offset_;

  static std::vector<SizeType> ConcatCumulativeSum(std::vector<SizeType> inp)
  {
    std::vector<SizeType> result(inp.size());
    result[0] = inp[0];
    for (SizeType i = 1; i < result.size(); i++)
    {
      result[i] = result[i - 1] + inp[i];
    }
    return result;
  }

  static SizeType ConcatGetArrayPositions(SizeType pos, std::vector<SizeType> array_sizes_cumsum)
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

  static bool ConcatTensorShapesValid(std::vector<SelfType> tensors, SizeType concat_axis)
  {
    std::vector<SizeType> n_dims(tensors.size());

    for (SizeType i = 0; i < tensors.size(); i++)
    {
      n_dims[i] = tensors[i].shape().size();
    }

    bool xxx{(std::equal(n_dims.begin() + 1, n_dims.end(), n_dims.begin()))};

    if (!std::equal(n_dims.begin() + 1, n_dims.end(), n_dims.begin()))
    {
      return false;
    }

    for (SizeType d = 0; d < tensors[0].shape().size(); d++)
    {
      if (d != concat_axis)
      {
        std::vector<SizeType> dim_sizes(tensors.size());
        for (SizeType i = 0; i < tensors.size(); i++)
        {
          dim_sizes[i] = tensors[i].shape()[d];
        }
        if (!std::equal(dim_sizes.begin() + 1, dim_sizes.end(), dim_sizes.begin()))
        {
          return false;
        };
      }
    }
    return true;
  }

  static std::vector<SizeType> ConcatInferShapesOfResultingTensor(std::vector<SelfType> tensors,
                                                                  SizeType              axis)
  {
    std::vector<SizeType> final_shape(tensors[0].shape().size());

    for (SizeType i = 0; i < tensors[0].shape().size(); i++)
    {
      if (i != axis)
      {
        final_shape[i] = tensors[0].shape()[i];
      }
      else
      {
        for (SizeType j = 0; j < tensors.size(); j++)
        {
          final_shape[i] += tensors[j].shape()[i];
        }
      }
    }

    return final_shape;
  }

  static std::vector<SizeType> ConcatGetDimsAlongAxisSummed(std::vector<SelfType> tensors,
                                                            SizeType              axis)
  {
    std::vector<SizeType> dim_along_concat_ax(tensors.size());

    for (SizeType i = 0; i < tensors.size(); i++)
    {
      dim_along_concat_ax[i] = tensors[i].shape()[axis];
    }

    std::vector<SizeType> dim_along_concat_ax_csummed{ConcatCumulativeSum(dim_along_concat_ax)};

    return dim_along_concat_ax_csummed;
  }

  static void ConcatAssignValues(SelfType &res_tensor, std::vector<SelfType> tensors,
                                 SizeType concat_axis, std::vector<SizeType> &counter)
  {
    std::vector<SizeType> tensors_concat_dim_info{
        ConcatGetDimsAlongAxisSummed(tensors, concat_axis)},
        counter_copy(counter);
    SizeType pos_in_n_arr,
        n_arr{ConcatGetArrayPositions(counter[concat_axis], tensors_concat_dim_info)};

    for (SizeType i = 0; i < counter.size(); ++i)
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

  static void ConcatRecursiveDimensionLookup(SelfType &res_tensor, std::vector<SelfType> tensors,
                                             SizeType concat_axis, std::vector<SizeType> counter,
                                             SizeType no_dim)
  {
    if (no_dim != res_tensor.shape().size())
    {
      for (counter[no_dim] = 0; counter[no_dim] < res_tensor.shape()[no_dim]; ++counter[no_dim])
      {
        ConcatRecursiveDimensionLookup(res_tensor, tensors, concat_axis, counter, (1 + no_dim));
      }
    }
    else
    {
      ConcatAssignValues(res_tensor, tensors, concat_axis, counter);
    }
  }

  static SelfType ConcatImplementation(std::vector<SelfType> tensors, SizeType concat_axis)
  {
    std::vector<SizeType> res_tensor_shape{
        ConcatInferShapesOfResultingTensor(tensors, concat_axis)};
    SelfType res_tensor(res_tensor_shape);

    ConcatRecursiveDimensionLookup(res_tensor, tensors, concat_axis,
                                   std::vector<SizeType>(res_tensor_shape.size(), 0), SizeType{0});

    return res_tensor;
  }
};

}  // namespace math
}  // namespace fetch
