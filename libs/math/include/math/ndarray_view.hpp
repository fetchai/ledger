#pragma once
#include "math/ndarray.hpp"

class NDArrayView
{
public:
  std::vector<std::size_t> from;
  std::vector<std::size_t> to;
  std::vector<std::size_t> step;

  // variables for copying view
  std::size_t cur_dim;
  std::vector<std::size_t> input_idxs;
  std::vector<std::size_t> output_idxs;
  std::size_t output_dims;

  NDArrayView(){}

  /**
   * iterates through the arrayview copying data from one array to another
   *
   * @param dest the NDArray to copy to
   * @param cur_dim
   */
  template <typename ArrayType>
  void recursive_copy(ArrayType &dest, ArrayType const &source)
  {
    cur_dim = from.size() - 1;
    reset_idxs();

    bool still_copying = true;
    int counter = 0;
    while (still_copying)
    {
      descend_and_incr_idx(dest, source);
      still_copying = ascend_and_incr_idx();
      counter += 1;
    }
  }

 private:

  /**
   * resets input and output indices and dimensions size
   */
  void reset_idxs()
  {
    // prepare initial input and output idxs
    for (std::size_t i = 0; i < from.size(); ++i)
    {
      input_idxs.push_back(from[i]);
      output_idxs.push_back(0);
    }

    output_dims = from.size();
  }

  /**
   * recursively ascend dimensions until we can find a valid dimension to increment
   *
   * after finished copying all data by iterating through final dimension, ascend until we find the next dimension to
   * increment
   *
   * @return still_copying    indicates whether there is more work to do or whether to finish
   */
  bool ascend_and_incr_idx()
  {
    bool still_copying = true;

    // check if entire cycle is complete
    if (cur_dim == 0)
    {
      still_copying = false;
      return still_copying;
    }

    // ascend one dimension
    cur_dim -= 1;

    // check if index can be incremented within bounds
    if (input_idxs[cur_dim] + step[cur_dim] <= to[cur_dim] - 1)
    {
      input_idxs[cur_dim] += step[cur_dim];
      output_idxs[cur_dim] += 1;
      return still_copying;
    }
    else
    {
      return ascend_and_incr_idx();
    }
  }

  /**
   * descends through all dimensions and reset indices as we go
   */
  void descend_and_reset()
  {
    while (cur_dim < output_dims - 1)
    {
      cur_dim += 1;
      input_idxs[cur_dim] = from[cur_dim];
      output_idxs[cur_dim] = 0;
    }
    return;
  }

  /**
   * descend towards trailing dimension and reset indices as we go
   *
   * after ascending completes we'll descend through all the dimensions until we get to the trailing dimension where we
   * can perform index iterations and copy data
   *
   * @tparam ArrayType
   * @param dest
   * @param source
   */
  template <typename ArrayType>
  void descend_and_incr_idx(ArrayType &dest, ArrayType const &source)
  {
    typedef decltype(source[0]) data_type;

    // descending dims and reset idxs
    descend_and_reset();

    // increment index of final dimensions and copy data
    assert(cur_dim == (output_dims - 1));
    bool incrementing = true;
    while (incrementing)
    {
      // copy a data point
      if (input_idxs[cur_dim] <= to[cur_dim])
      {
        data_type new_val = data_type(source.operator()(input_idxs));
        dest.Assign(output_idxs, new_val);
      }

      // eith increment index of final dimension or switch to ascending if it would be out of bounds
      if (input_idxs[cur_dim] + step[cur_dim] <= to[cur_dim] - 1)
      {
        output_idxs[cur_dim] += 1;
        input_idxs[cur_dim] += step[cur_dim];

        incrementing = true;
      }
      else // can't increment this index anymore!
      {
        incrementing = false;
      }

    }
  }

};