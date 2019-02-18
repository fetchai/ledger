//#pragma once
////------------------------------------------------------------------------------
////
////   Copyright 2018-2019 Fetch.AI Limited
////
////   Licensed under the Apache License, Version 2.0 (the "License");
////   you may not use this file except in compliance with the License.
////   You may obtain a copy of the License at
////
////       http://www.apache.org/licenses/LICENSE-2.0
////
////   Unless required by applicable law or agreed to in writing, software
////   distributed under the License is distributed on an "AS IS" BASIS,
////   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
////   See the License for the specific language governing permissions and
////   limitations under the License.
////
////------------------------------------------------------------------------------
//
////#include "math/ndarray.hpp"
//#include <algorithm>
//#include <cassert>
//#include <vector>
////
//
// namespace fetch {
// namespace math {
//
//// template <typename T, typename C>
//// class ShapelessArray;
// template <typename T, typename C>
// class NDArray;
//
// struct NDIteratorRange
//{
//  std::size_t index       = 0;
//  std::size_t from        = 0;
//  std::size_t to          = 0;
//  std::size_t step        = 1;
//  std::size_t volume      = 1;
//  std::size_t total_steps = 1;
//
//  std::size_t step_volume  = 1;
//  std::size_t total_volume = 1;
//
//  std::size_t repeat_dimension = 1;
//  std::size_t repetition       = 0;
//
//  std::size_t current_n_dim_position = 0;
//};
//
// template <typename T, typename C>
// class NDArrayIterator
//{
// public:
//  using type         = T;
//  using ndarray_type = NDArray<T, C>;
//
//  /**
//   * default range assumes step 1 over whole array - useful for trivial cases
//   * @param array
//   */
//  NDArrayIterator(ndarray_type &array)
//    : array_(array)
//  {
//    std::vector<std::vector<std::size_t>> step{};
//    for (auto i : array.shape())
//    {
//      step.push_back({0, i, 1});
//    }
//    Setup(step, array_.shape());
//  }
//
//  /**
//   * Iterator for more interesting ranges
//   * @param array the NDArray to operate upon
//   * @param step the from,to,and step range objects
//   */
//  NDArrayIterator(ndarray_type &array, std::vector<std::vector<std::size_t>> const &step)
//    : array_(array)
//  {
//    Setup(step, array_.shape());
//  }
//
//  NDArrayIterator(ndarray_type &array, std::vector<std::size_t> const &shape)
//    : array_(array)
//  {
//    std::vector<std::vector<std::size_t>> step{};
//    for (auto i : array.shape())
//    {
//      step.push_back({0, i, 1});
//    }
//
//    Setup(step, shape);
//  }
//
//  /**
//   * identifies whether the iterator is still valid or has finished iterating
//   * @return boolean indicating validity
//   */
//  operator bool()
//  {
//    return counter_ < size_;
//  }
//
//  /**
//   * incrementer, i.e. increment through the memory by 1 position making n-dim adjustments as
//   * necessary
//   * @return
//   */
//  NDArrayIterator &operator++()
//  {
//    bool        next;
//    std::size_t i = 0;
//    ++counter_;
//    do
//    {
//
//      next               = false;
//      NDIteratorRange &s = ranges_[i];
//      s.index += s.step;
//      position_ += s.step_volume;
//      ++s.current_n_dim_position;
//
//      if (s.index >= s.to)
//      {
//
//        ++s.repetition;
//        s.index                  = s.from;
//        s.current_n_dim_position = s.from;
//        position_ -= s.total_volume;
//
//        if (s.repetition == s.repeat_dimension)
//        {
//          s.repetition = 0;
//          next         = true;
//          ++i;
//        }
//      }
//    } while ((i < ranges_.size()) && (next));
//
//    // check if iteration is complete
//    if (i == ranges_.size())
//    {
//      if (counter_ < size_)
//      {
//        --total_runs_;
//        position_ = 0;
//        for (auto &r : ranges_)
//        {
//          r.index = r.from;
//          position_ += r.volume * r.index;
//        }
//      }
//    }
//
//#ifndef NDEBUG
//    // Test
//
//    std::size_t ref = 0;
//    for (auto &s : ranges_)
//    {
//      ref += s.volume * s.index;
//    }
//
//    assert(ref == position_);
//#endif
//
//    return *this;
//  }
//
//  /**
//   * transpose axes according to the new order specified in perm
//   * @param perm
//   */
//  void Transpose(std::vector<std::size_t> const &perm)
//  {
//    std::vector<NDIteratorRange> new_ranges;
//    new_ranges.reserve(ranges_.size());
//    for (std::size_t i = 0; i < ranges_.size(); ++i)
//    {
//      new_ranges.push_back(ranges_[perm[i]]);
//    }
//    std::swap(new_ranges, ranges_);
//  }
//
//  void PermuteAxes(std::size_t const &a, std::size_t const &b)
//  {
//    std::swap(ranges_[a], ranges_[b]);
//  }
//
//  void MoveAxesToFront(std::size_t const &a)
//  {
//    std::vector<NDIteratorRange> new_ranges;
//    new_ranges.reserve(ranges_.size());
//    new_ranges.push_back(ranges_[a]);
//    for (std::size_t i = 0; i < ranges_.size(); ++i)
//    {
//      if (i != a)
//      {
//        new_ranges.push_back(ranges_[i]);
//      }
//    }
//    std::swap(new_ranges, ranges_);
//  }
//
//  void ReverseAxes()
//  {
//    std::reverse(ranges_.begin(), ranges_.end());
//  }
//
//  /**
//   * dereference, i.e. give the value at the current position of the iterator
//   * @return
//   */
//  type &operator*()
//  {
//    assert(position_ < array_.size());
//
//    return array_[position_];
//  }
//
//  type const &operator*() const
//  {
//    return array_[position_];
//  }
//
//  std::size_t size() const
//  {
//    return size_;
//  }
//
//  template <typename A, typename B>
//  friend bool UpgradeIteratorFromBroadcast(std::vector<std::size_t> const &,
//                                           NDArrayIterator<A, B> &);
//
//  /**
//   * returns the n-dimensional index of the current position
//   * @return
//   */
//  std::vector<std::size_t> GetNDimIndex()
//  {
//    std::vector<std::size_t> cur_index;
//    for (std::size_t j = 0; j < ranges_.size(); ++j)
//    {
//      cur_index.push_back(ranges_[j].current_n_dim_position);
//    }
//
//    return cur_index;
//  }
//
//  NDIteratorRange const &range(std::size_t const &i)
//  {
//    return ranges_[i];
//  }
//
// protected:
//  std::vector<NDIteratorRange> ranges_;
//  std::size_t                  total_runs_ = 1;
//  std::size_t                  size_       = 0;
//
// private:
//  void Setup(std::vector<std::vector<std::size_t>> const &step,
//             std::vector<std::size_t> const &             shape)
//  {
//    assert(array_.shape().size() == step.size());
//    std::size_t volume = 1;
//    size_              = 1;
//    position_          = 0;
//
//    for (std::size_t i = 0; i < step.size(); ++i)
//    {
//      auto const &    a = step[i];
//      NDIteratorRange s;
//      s.index = s.from = s.current_n_dim_position = a[0];
//      s.to                                        = a[1];
//
//      if (a.size() > 2)
//      {
//        s.step = a[2];
//      }
//      s.volume         = volume;
//      std::size_t diff = (s.to - s.from);
//      s.total_steps    = diff / s.step;
//      if (s.total_steps * s.step < diff)
//      {
//        ++s.total_steps;
//      }
//
//      s.total_steps *= s.step;
//      s.step_volume  = s.step * volume;
//      s.total_volume = (s.total_steps) * volume;
//
//      position_ += volume * s.from;
//      size_ *= s.total_steps;
//
//      volume *= shape[i];
//      ranges_.push_back(s);
//    }
//  }
//
//  ndarray_type &array_;
//  std::size_t   position_ = 0;
//
//  std::size_t counter_ = 0;
//};
//
//}  // namespace math
//}  // namespace fetch
