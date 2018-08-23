#pragma once
#include "math/ndarray.hpp"
#include <vector>

//
////// need to forward declare
////class NDArray;

namespace fetch {
namespace math {

template <typename T, typename C>
class ShapeLessArray;
template <typename T, typename C>
class NDArray;

struct NDIteratorRange
{
  std::size_t index       = 0;
  std::size_t from        = 0;
  std::size_t to          = 0;
  std::size_t step        = 1;
  std::size_t volume      = 1;
  std::size_t total_steps = 1;

  std::size_t step_volume  = 1;
  std::size_t total_volume = 1;

  std::size_t repeat_dimension = 1;
  std::size_t repetition       = 0;

  std::size_t current_n_dim_position = 0;
};

template <typename T, typename C>
class NDArrayIterator
{
public:
  using type         = T;
  using ndarray_type = NDArray<T, C>;

  /**
   * default range assumes step 1 over whole array - useful for trivial cases
   * @param array
   */
  NDArrayIterator(ndarray_type &array) : array_(array)
  {
    std::vector<std::vector<std::size_t>> step{};
    for (auto i : array.shape())
    {
      step.push_back({0, i, 1});
    }
    Setup(step);
  }

  /**
   * Iterator for more interesting ranges
   * @param array the NDArray to operate upon
   * @param step the from,to,and step range objects
   */
  NDArrayIterator(ndarray_type &array, std::vector<std::vector<std::size_t>> const &step)
    : array_(array)
  {
    Setup(step);
  }

  /**
   * identifies whether the iterator is still valid or has finished iterating
   * @return boolean indicating validity
   */
  operator bool() { return counter_ < size_; }

  /**
   * incrementer, i.e. increment through the memory by 1 position making n-dim adjustments as
   * necessary
   * @return
   */
  NDArrayIterator &operator++()
  {
    bool        next;
    std::size_t i = 0;
    ++counter_;
    do
    {

      next               = false;
      NDIteratorRange &s = ranges_[i];
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
    // Test

    std::size_t ref = 0;
    for (auto &s : ranges_)
    {
      ref += s.volume * s.index;
    }

    if (ref != position_)
    {
      std::cout << "Expected " << ref << " but got " << position_ << std::endl;
      TODO_FAIL("doesn't add up");
    }
    assert(ref == position_);
#endif

    return *this;
  }

  void PermuteAxes(std::size_t const &a, std::size_t const &b)
  {
    std::swap(ranges_[a], ranges_[b]);
  }

  void MoveAxesToFront(std::size_t const &a)
  {
    std::vector<NDIteratorRange> new_ranges;
    new_ranges.reserve(ranges_.size());
    new_ranges.push_back(ranges_[a]);
    for (std::size_t i = 0; i < ranges_.size(); ++i)
    {
      if (i != a)
      {
        new_ranges.push_back(ranges_[i]);
      }
    }
    std::swap(new_ranges, ranges_);
  }

  /**
   * dereference, i.e. give the value at the current position of the iterator
   * @return
   */
  type &operator*()
  {
    assert(position_ < array_.size());

    return array_[position_];
  }

  type const &operator*() const { return array_[position_]; }

  std::size_t size() const { return size_; }

  template <typename A, typename B>
  friend bool UpgradeIteratorFromBroadcast(std::vector<std::size_t> const &,
                                           NDArrayIterator<A, B> &);

  /**
   * returns the n-dimensional index of the current position
   * @return
   */
  std::vector<std::size_t> GetNDimIndex()
  {
    std::vector<std::size_t> cur_index;
    for (std::size_t j = 0; j < ranges_.size(); ++j)
    {
      cur_index.push_back(ranges_[j].current_n_dim_position);
    }

    return cur_index;
  }

  NDIteratorRange const &range(std::size_t const &i) { return ranges_[i]; }

protected:
  std::vector<NDIteratorRange> ranges_;
  std::size_t                  total_runs_ = 1;
  std::size_t                  size_       = 0;

private:
  void Setup(std::vector<std::vector<std::size_t>> const &step)
  {
    assert(array_.shape().size() == step.size());
    std::size_t volume = 1;
    size_              = 1;
    position_          = 0;

    for (std::size_t i = 0; i < step.size(); ++i)
    {
      auto const &    a = step[i];
      NDIteratorRange s;
      s.index = s.from = s.current_n_dim_position = a[0];
      s.to                                        = a[1];

      if (a.size() > 2)
      {
        s.step = a[2];
      }
      s.volume         = volume;
      std::size_t diff = (s.to - s.from);
      s.total_steps    = diff / s.step;
      if (s.total_steps * s.step < diff) ++s.total_steps;

      s.total_steps *= s.step;
      s.step_volume  = s.step * volume;
      s.total_volume = (s.total_steps) * volume;

      position_ += volume * s.from;
      size_ *= s.total_steps;

      volume *= array_.shape(i);
      ranges_.push_back(s);
    }
  }

  ndarray_type &array_;
  std::size_t   position_ = 0;

  std::size_t counter_ = 0;
};

}  // namespace math
}  // namespace fetch
