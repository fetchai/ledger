#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "math/free_functions/metrics/metrics.hpp"
#include "math/meta/type_traits.hpp"
#include "random"
#include <vector>

/**
 * assigns the absolute of x to this array
 * @param x
 */

namespace fetch {
namespace math {
namespace clustering {

namespace details {

template <typename ArrayType>
class KMeansImplementation
{
public:
  std::size_t n_points;
  std::size_t n_dimensions;
  std::size_t n_clusters;

  bool   not_converged = true;
  double running_mean;  // we'll use this find the smallest euclidean distance out of K comparisons

  std::default_random_engine rng;

  std::vector<std::size_t> data_idxs;       // a vector of indices to the data used for shuffling
  std::vector<std::size_t> empty_clusters;  // a vector tracking whenever a cluster goes empty

  ArrayType k_means;       // current cluster centres
  ArrayType prev_k_means;  // previous cluster centres (for checking convergence)
  ArrayType temp_k;        // a container for ease of access to using Euclidean function

  ArrayType k_assignment;       // current data to cluster assignment
  ArrayType prev_k_assignment;  // previous data to cluster assignment (for checkign convergence)
  std::vector<std::size_t> reassigned_k;  // reassigned data to cluster assignment

  std::vector<std::size_t>
                         k_count;  // count of how many data points per cluster (for checking reassignment)
  std::vector<ArrayType> k_euclids;  // container for current euclid distances

  std::size_t assigned_k;  // current cluster to assign

  bool reassign;

  bool        max_loops_not_reached = true;
  std::size_t loop_counter          = 0;
  std::size_t max_loops;

  KMeansImplementation(ArrayType const &data, std::size_t n_clusters, ArrayType &ret,
                       std::size_t max_loops = 100)
    : n_clusters(n_clusters)
    , max_loops(max_loops)
  {
    n_points     = data.shape()[0];
    n_dimensions = data.shape()[1];

    // initialise k means
    std::vector<std::size_t> k_means_shape{n_clusters, n_dimensions};
    k_means      = ArrayType(k_means_shape);
    prev_k_means = ArrayType::Zeroes(k_means_shape);
    temp_k       = ArrayType(data.shape());

    data_idxs = std::vector<std::size_t>(n_points);  // vector with 100 ints.
    std::iota(std::begin(data_idxs), std::end(data_idxs), 0);
    std::shuffle(data_idxs.begin(), data_idxs.end(), rng);
    for (std::size_t i = 0; i < n_clusters; ++i)
    {
      for (std::size_t j = 0; j < n_dimensions; ++j)
      {
        k_means.Set(i, j, data.At(data_idxs[i], j));
      }
    }

    // instantiate counter with zeros
    k_count = std::vector<std::size_t>{0, n_clusters};

    // initialise assignment
    std::vector<std::size_t> k_assignment_shape{n_points, 1};
    k_assignment =
        ArrayType(k_assignment_shape);  // it's okay not to initialise as long as we take the
    prev_k_assignment = ArrayType(
        k_assignment_shape);  // need to keep a record of previous to check for convergence
    for (std::size_t l = 0; l < prev_k_assignment.size(); ++l)
    {
      prev_k_assignment.Set(l, -1);
    }

    // initialise size of euclidean distance container
    k_euclids      = std::vector<ArrayType>(n_clusters);
    empty_clusters = std::vector<std::size_t>(n_clusters);

    ///////////////////////
    /// COMPUTE K MEANS ///
    ///////////////////////
    while (NotConverged())
    {
      Assign(data);
      Update(data);
    }
    UnReassign();

    ret = k_assignment;  // assign the final output
  };

  /**
   * part 1 of the iterative process for KMeans:
   * Assign each data point to a cluster based on euclidean distance
   */
  void Assign(ArrayType const &data)
  {
    // replicate kmeans which is 1 x n_dims into n_data x n_dims
    // allows for easy call to EuclideanDistance
    for (std::size_t i = 0; i < n_clusters; ++i)
    {
      for (std::size_t j = 0; j < n_points; ++j)
      {
        for (std::size_t k = 0; k < n_dimensions; ++k)
        {
          temp_k.Set(j, k, k_means.At(i, k));
        }
      }
      k_euclids[i] = fetch::math::metrics::EuclideanDistance(data, temp_k, 1);
    }

    // now we have a vector of n_data x 1 Arrays
    // we have to go through and compare which is smallest for each K and make the assignment
    for (std::size_t i = 0; i < n_points; ++i)
    {
      running_mean = std::numeric_limits<double>::max();
      for (std::size_t j = 0; j < n_clusters; ++j)
      {
        if (k_euclids[j][i] < running_mean)
        {
          running_mean = k_euclids[j][i];
          assigned_k   = j;
        }
      }
      k_assignment.Set(i, 0, assigned_k);
      ++k_count[assigned_k];
    }

    // sometimes we get an empty cluster - in these cases we should reassign one data point to that
    // cluster
    Reassign();
  }

  /**
   * Method assigns a random data point to each empty cluster
   */
  void Reassign()
  {
    // search for empty clusters
    reassign = false;
    std::fill(empty_clusters.begin(), empty_clusters.end(), 0);
    for (std::size_t i = 0; i < n_clusters; ++i)
    {
      if (k_count[i] == 0)
      {
        empty_clusters[i] = 1;
        reassign          = true;
      }
    }

    // if a category has been completely eliminated! we should randomly assign one data point to it
    if (reassign)
    {
      std::fill(reassigned_k.begin(), reassigned_k.end(), 0);
      std::shuffle(data_idxs.begin(), data_idxs.end(), rng);

      for (std::size_t i = 0; i < n_clusters; ++i)
      {
        if (empty_clusters[i] == 1)
        {
          reassigned_k[i] = static_cast<std::size_t>(k_assignment[i]);
          k_assignment[i] = data_idxs[i];
          ++k_count[i];
        }
      }
    }
  }

  /**
   * Method reverts reassigned data because convergence is complete
   */
  void UnReassign()
  {
    if (reassign)
    {
      for (std::size_t i = 0; i < n_clusters; ++i)
      {
        if (empty_clusters[i] == 1)
        {
          k_assignment[i] = reassigned_k[i];
        }
      }
    }
  }

  /**
   * part 2 of the iterative process for KMeans:
   * update the cluster centres
   */
  void Update(ArrayType const &data)
  {
    std::fill(k_means.begin(), k_means.end(), 0);
    // get KSums
    for (std::size_t i = 0; i < n_points; ++i)
    {
      auto cur_k = static_cast<std::size_t>(k_assignment[i]);
      for (std::size_t j = 0; j < n_dimensions; ++j)
      {
        k_means.Set(cur_k, j, k_means.At(cur_k, j) + data.At(i, j));
      }
    }

    // divide sums to get KMeans
    for (std::size_t m = 0; m < n_clusters; ++m)
    {
      for (std::size_t i = 0; i < n_dimensions; ++i)
      {
        k_means.Set(m, i, k_means.At(m, i) / k_count[m]);
      }
    }
  }

  /**
   * checks for convergence of iterative algorithm
   */
  bool NotConverged()
  {
    // completed max n loops
    if (loop_counter >= max_loops)
    {
      return false;
    }
    ++loop_counter;

    // last update changed nothing (i.e converged)
    if ((k_assignment == prev_k_assignment) && (k_means == prev_k_means))
    {
      return false;
    }
    prev_k_assignment.Copy(k_assignment);
    prev_k_means.Copy(k_means);

    return true;
  }
};

}  // namespace details

template <typename ArrayType>
ArrayType KMeans(ArrayType const &data, std::size_t K)
{
  std::size_t n_points = data.shape()[0];

  // we can't have more clusters than data points
  assert(K <= n_points);  // you can't have more clusters than data points
  assert(K > 1);          // why would you run k means clustering with only one cluster?

  std::vector<std::size_t> ret_array_shape{n_points, 1};
  ArrayType                ret{ret_array_shape};

  if (n_points == K)  // very easy to cluster!
  {
    for (std::size_t i = 0; i < n_points; ++i)
    {
      ret[i] = i;
    }
  }
  else  // real work happens in these cases
  {
    details::KMeansImplementation<ArrayType>(data, K, ret);
  }

  return ret;
}

}  // namespace clustering
}  // namespace math
}  // namespace fetch
