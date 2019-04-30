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

#include "math/base_types.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

namespace fetch {
namespace math {

// need to forward declare
template <typename T, typename C>
class Tensor;

struct TensorIteratorRange
{
  using SizeType       = uint64_t;
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
class TensorIterator
{
public:
  using Type     = T;
  using SizeType = uint64_t;
  /**
   * default range assumes step 1 over whole array - useful for trivial cases
   * @param array
   */
  TensorIterator(TensorType &array)
    : array_(array)
  {
    std::vector<std::vector<SizeType>> step{};
    for (auto i : array.shape())
    {
      step.push_back({0, i, 1});
    }
    Setup(step, array_.stride());
  }

  TensorIterator(TensorIterator const &other) = default;
  TensorIterator &operator=(TensorIterator const &other) = default;
  TensorIterator(TensorIterator &&other)                 = default;
  TensorIterator &operator=(TensorIterator &&other) = default;

  /**
   * Iterator for more interesting ranges
   * @param array the Tensor to operate upon
   * @param step the from,to,and step range objects
   */
  TensorIterator(TensorType &array, std::vector<std::vector<SizeType>> const &step)
    : array_(array)
  {
    Setup(step, array_.stride());
  }

  static TensorIterator EndIterator(TensorType &array)
  {
    auto ret     = TensorIterator(array);
    ret.counter_ = ret.size_;
    return ret;
  }

  TensorIterator(TensorType &array, std::vector<SizeType> const &stride)
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
  operator bool() const
  {
    return is_valid();
  }

  /**
   * incrementer, i.e. increment through the memory by 1 position making n-dim adjustments as
   * necessary
   * @return
   */
  TensorIterator &operator++()
  {
    bool     next;
    SizeType i = 0;
    ++counter_;
    do
    {
      next                   = false;
      TensorIteratorRange &s = ranges_[i];
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
    std::vector<TensorIteratorRange> new_ranges;
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

  // TODO: Name correctly
  void MoveAxesToFront(SizeType const &a)
  {
    std::vector<TensorIteratorRange> new_ranges;
    new_ranges.reserve(ranges_.size());
    new_ranges.push_back(ranges_[a]);
    for (SizeType i = 0; i < ranges_.size(); ++i)
    {
      if (i != a)
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
    assert(position_ < array_.size());

    return array_[position_];
  }

  Type const &operator*() const
  {
    return array_[position_];
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

  template <typename A, typename B>
  friend bool UpgradeIteratorFromBroadcast(std::vector<SizeType> const &, TensorIterator<A, B> &);

  /**
   * returns the n-dimensional index of the current position
   * @return
   */
  std::vector<SizeType> GetNDimIndex()
  {
    std::vector<SizeType> cur_index;
    for (SizeType j = 0; j < ranges_.size(); ++j)
    {
      cur_index.push_back(ranges_[j].current_n_dim_position);
    }

    return cur_index;
  }

  TensorIteratorRange const &range(SizeType const &i)
  {
    return ranges_[i];
  }

  bool operator==(TensorIterator const &other) const
  {
    if (this->end_of_iterator() && other->end_of_iterator())
    {
      return true;
    }
    return other.counter_ == counter_;
  }

  bool operator!=(TensorIterator const &other) const
  {
    return other.counter_ != counter_;
  }

protected:
  std::vector<TensorIteratorRange> ranges_;
  SizeType                         total_runs_ = 1;
  SizeType                         size_       = 0;

private:
  void Setup(std::vector<std::vector<SizeType>> const &step, std::vector<SizeType> const &stride)
  {
    ASSERT(array_.shape().size() == step.size());
    SizeType volume = 1;

    if (step.size() == 0)
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
        auto const &        a = step[i];
        TensorIteratorRange s;
        s.index = s.from = s.current_n_dim_position = a[0];
        s.to                                        = a[1];

        if (a.size() > 2)
        {
          s.step = a[2];
        }
        volume = stride[i];
                
        s.volume      = volume;
        SizeType diff = (s.to - s.from);
        s.total_steps = diff / s.step;
        if (s.total_steps * s.step < diff)
        {
          ++s.total_steps;
        }

        s.total_steps *= s.step;
        s.step_volume  = s.step * volume;
        s.total_volume = (s.total_steps) * volume;

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
using ConstTensorIterator = TensorIterator<T const, C, Tensor<T, C> const>;

}  // namespace math
}  // namespace fetch
