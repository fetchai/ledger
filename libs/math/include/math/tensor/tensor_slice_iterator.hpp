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

#include "math/base_types.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

namespace fetch {
namespace math {

struct TensorSliceIteratorRange
{
  using SizeType       = fetch::math::SizeType;
  SizeType index       = 0;
  SizeType from        = 0;
  SizeType to          = 0;
  SizeType step        = 1;
  SizeType volume      = 1;
  SizeType total_steps = 1;

  SizeType step_volume  = 1;
  SizeType total_volume = 1;

  SizeType repeat_dimension = 1;
  SizeType repetition       = 0;

  SizeType current_n_dim_position = 0;
};

template <typename T, typename C, typename TensorType = Tensor<T, C>>
class TensorSliceIterator
{
public:
  using Type     = T;
  using SizeType = fetch::math::SizeType;
  /**
   * default range assumes step 1 over whole array - useful for trivial cases
   * @param array
   */
  explicit TensorSliceIterator(TensorType &array)
    : array_(array)
  {
    std::vector<std::vector<SizeType>> step{};
    for (auto i : array.shape())
    {
      step.push_back({0, i, 1});
    }
    Setup(step, array_.stride());
  }

  TensorSliceIterator(TensorSliceIterator const &other) = default;
  TensorSliceIterator &operator=(TensorSliceIterator const &other) = default;
  TensorSliceIterator(TensorSliceIterator &&other) noexcept        = default;
  TensorSliceIterator &operator=(TensorSliceIterator &&other) noexcept = default;

  /**
   * Iterator for more interesting ranges
   * @param array the Tensor to operate upon
   * @param step the from,to,and step range objects
   */
  TensorSliceIterator(TensorType &array, std::vector<std::vector<SizeType>> const &step)
    : array_(array)
  {
    Setup(step, array_.stride());
  }

  static TensorSliceIterator EndIterator(TensorType &array)
  {
    auto ret     = TensorSliceIterator(array);
    ret.counter_ = ret.size_;
    return ret;
  }

  TensorSliceIterator(TensorType &array, std::vector<SizeType> const &stride)
    : array_(array)
  {
    std::vector<std::vector<SizeType>> step{};
    for (auto i : array.shape())
    {
      step.push_back({0, i, 1});
    }

    Setup(step, stride);
  }

  /**
   * identifies whether the iterator is still valid or has finished iterating
   * @return boolean indicating validity
   */
  bool is_valid() const
  {
    return counter_ < size_;
  }

  /**
   * same as is_valid
   * @return
   */
  explicit operator bool() const
  {
    return is_valid();
  }

  /**
   * incrementer, i.e. increment through the memory by 1 position making n-dim adjustments as
   * necessary
   * @return
   */
  TensorSliceIterator &operator++()
  {
    bool     next;
    SizeType i = 0;

    ++counter_;
    do
    {
      next = false;
      assert(i < ranges_.size());
      TensorSliceIteratorRange &s = ranges_[i];
      s.index += s.step;
      position_ += s.step_volume;
      ++s.current_n_dim_position;

      if (s.index >= s.to)
      {
        ++s.repetition;
        s.index                  = s.from;
        s.current_n_dim_position = s.from;
        position_ -= s.total_volume;

        if (s.repetition == s.repeat_dimension)
        {
          s.repetition = 0;
          next         = true;
          ++i;
        }
      }
    } while ((i < ranges_.size()) && (next));

    // check if iteration is complete
    if (i == ranges_.size())
    {
      if (counter_ < size_)
      {
        --total_runs_;
        position_ = 0;
        for (auto &r : ranges_)
        {
          r.index = r.from;
          position_ += r.volume * r.index;
        }
      }
    }

#ifndef NDEBUG
    SizeType ref = 0;
    for (auto &s : ranges_)
    {
      ref += s.volume * s.index;
    }

    assert(ref == position_);
#endif

    return *this;
  }

  /**
   * transpose axes according to the new order specified in perm
   * @param perm
   */
  void Transpose(std::vector<SizeType> const &perm)
  {
    std::vector<TensorSliceIteratorRange> new_ranges;
    new_ranges.reserve(ranges_.size());
    for (SizeType i = 0; i < ranges_.size(); ++i)
    {
      new_ranges.push_back(ranges_[perm[i]]);
    }
    std::swap(new_ranges, ranges_);
  }

  void PermuteAxes(SizeType const &a, SizeType const &b)
  {
    std::swap(ranges_[a], ranges_[b]);
  }

  /**
   * Permutes ranges_ where specified ranges in axes vector are moved to front
   * old ranges_{r0,r1.r2,r3} with axes{3,2}
   * Will result in ranges_ {r3.r2,r0,r1}
   * @param axes vector of axes indexes
   */
  void MoveAxesToFront(std::vector<SizeType> const &axes)
  {
    std::vector<TensorSliceIteratorRange> new_ranges;
    new_ranges.reserve(ranges_.size());

    // Insert axes at beginning
    for (uint64_t axis : axes)
    {
      new_ranges.push_back(ranges_[axis]);
    }

    for (SizeType i = 0; i < ranges_.size(); ++i)
    {
      // Search for axis
      bool add_axis = true;
      for (uint64_t axis : axes)
      {
        if (i == axis)
        {
          add_axis = false;
          break;
        }
      }
      // Add axis if wasn't added at beginning
      if (!add_axis)
      {
        continue;
      }
      new_ranges.push_back(ranges_[i]);
    }
    std::swap(new_ranges, ranges_);
  }

  /**
   * Permutes ranges_ where range at specified axis index is moved to front
   * old ranges_{r0,r1.r2,r3} with axis=2
   * Will result in ranges_ {r2.r0,r1,r3}
   * @param index of range to be moved to front in range_ vector
   */
  void MoveAxisToFront(SizeType const &axis)
  {
    std::vector<TensorSliceIteratorRange> new_ranges;
    new_ranges.reserve(ranges_.size());
    new_ranges.push_back(ranges_[axis]);
    for (SizeType i = 0; i < ranges_.size(); ++i)
    {
      if (i != axis)
      {
        new_ranges.push_back(ranges_[i]);
      }
    }
    std::swap(new_ranges, ranges_);
  }

  void ReverseAxes()
  {
    std::reverse(ranges_.begin(), ranges_.end());
  }

  /**
   * dereference, i.e. give the value at the current position of the iterator
   * @return
   */
  Type &operator*()
  {
    assert(position_ < array_.padded_size());
    return array_.data()[position_];
  }

  Type const &operator*() const
  {
    assert(position_ < array_.padded_size());
    return array_.data()[position_];
  }

  SizeType position() const
  {
    return position_;
  }

  SizeType PositionAlong(SizeType axis) const
  {
    return ranges_[axis].current_n_dim_position;
  }

  SizeVector PositionVector() const
  {
    SizeVector ret;
    for (auto const &r : ranges_)
    {
      ret.push_back(r.current_n_dim_position);
    }
    return ret;
  }

  SizeType size() const
  {
    return size_;
  }

  SizeType counter() const
  {
    return counter_;
  }

  template <class IteratorType>
  // NOLINTNEXTLINE
  friend bool UpgradeIteratorFromBroadcast(std::vector<SizeType> const &, IteratorType &);

  /**
   * returns the n-dimensional index of the current position
   * @return
   */
  std::vector<SizeType> GetIndex()
  {
    std::vector<SizeType> cur_index;
    for (SizeType j = 0; j < ranges_.size(); ++j)
    {
      cur_index.push_back(ranges_[j].current_n_dim_position);
    }

    return cur_index;
  }

  TensorSliceIteratorRange const &range(SizeType const &i)
  {
    return ranges_[i];
  }

  bool operator==(TensorSliceIterator const &other) const
  {
    if (this->end_of_iterator() && other->end_of_iterator())
    {
      return true;
    }
    return other.counter_ == counter_;
  }

  bool operator!=(TensorSliceIterator const &other) const
  {
    return other.counter_ != counter_;
  }

protected:
  std::vector<TensorSliceIteratorRange> ranges_;
  SizeType                              total_runs_ = 1;
  SizeType                              size_       = 0;

private:
  void Setup(std::vector<std::vector<SizeType>> const &step, std::vector<SizeType> const &stride)
  {
    assert(array_.shape().size() == step.size());
    SizeType volume = 1;

    if (step.empty())
    {
      size_     = 0;
      position_ = 0;
    }
    else
    {
      size_     = 1;
      position_ = 0;

      for (SizeType i = 0; i < step.size(); ++i)
      {
        auto const &             a = step[i];
        TensorSliceIteratorRange s;
        s.index = s.from = s.current_n_dim_position = a[0];
        s.to                                        = a[1];

        if (a.size() > 2)
        {
          s.step = a[2];
        }
        volume = stride[i];

        s.volume      = volume;
        s.total_steps = ((s.to - s.from - 1) / s.step) + 1;

        s.step_volume  = s.step * volume;
        s.total_volume = (s.total_steps) * s.step_volume;

        position_ += volume * s.from;
        size_ *= s.total_steps;

        ranges_.push_back(s);
      }
    }
  }

  TensorType &array_;
  SizeType    position_ = 0;

  SizeType counter_ = 0;
};

template <typename T, typename C>
using ConstTensorSliceIterator = TensorSliceIterator<T const, C, Tensor<T, C> const>;

}  // namespace math
}  // namespace fetch
