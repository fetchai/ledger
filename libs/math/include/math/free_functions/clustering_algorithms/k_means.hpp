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

#include "math/free_functions/free_functions.hpp"
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
  /**
   * The constructor for the KMeans implementation object
   * @param data the data itself in the format of a 2D array of n_points x n_dims
   * @param n_clusters the number K of clusters to identify
   * @param ret the return matrix of shape n_points x 1 with values in range 0 -> K-1
   * @param r_seed a random seed for the data shuffling
   * @param max_loops maximum number of loops before assuming convergence
   * @param init_mode what type of initialization to use
   */
  KMeansImplementation(ArrayType const &data, std::size_t const &n_clusters, ArrayType &ret,
                       std::size_t const &r_seed, std::size_t const &max_loops,
                       std::size_t init_mode = 0, std::size_t max_no_change_convergence = 10)
    : n_clusters_(n_clusters)
    , max_no_change_convergence_(max_no_change_convergence)
    , max_loops_(max_loops)
    , init_mode_(init_mode)
  {
    // seed random number generator
    rng_.seed(r_seed);

    n_points_     = data.shape()[0];
    n_dimensions_ = data.shape()[1];

    // initialise k means
    std::vector<std::size_t> k_means_shape{n_clusters_, n_dimensions_};
    k_means_      = ArrayType(k_means_shape);
    prev_k_means_ = ArrayType::Zeroes(k_means_shape);
    temp_k_       = ArrayType(data.shape());

    InitialiseKMeans(data);

    // instantiate counter with zeros
    k_count_ = std::vector<std::size_t>{0, n_clusters_};

    // initialise assignment
    std::vector<std::size_t> k_assignment_shape{n_points_, 1};
    k_assignment_ =
        ArrayType(k_assignment_shape);  // it's okay not to initialise as long as we take the
    prev_k_assignment_ = ArrayType(
        k_assignment_shape);  // need to keep a record of previous to check for convergence
    for (std::size_t l = 0; l < prev_k_assignment_.size(); ++l)
    {
      prev_k_assignment_.Set(l, -1);
    }

    // initialise size of euclidean distance container
    k_euclids_      = std::vector<ArrayType>(n_clusters_);
    empty_clusters_ = std::vector<std::size_t>(n_clusters_);

    ///////////////////////
    /// COMPUTE K MEANS ///
    ///////////////////////
    while (NotConverged())
    {
      Assign(data);
      Update(data);
    }
    UnReassign();

    ret = k_assignment_;  // assign the final output
  };

private:
  void InitialiseKMeans(ArrayType const &data)
  {
    // shuffle the data
    data_idxs_ = std::vector<std::size_t>(n_points_);
    std::iota(std::begin(data_idxs_), std::end(data_idxs_), 0);
    std::shuffle(data_idxs_.begin(), data_idxs_.end(), rng_);

    switch (init_mode_)
    {
    case InitMode::KMeansPP:
    {
      // assign first cluster centre
      for (std::size_t j = 0; j < n_dimensions_; ++j)
      {
        k_means_.Set(0, j, data.At(data_idxs_[0], j));
      }

      // assign remaining cluster centres
      std::size_t n_remaining_data_points, n_assigned_data_points;
      std::size_t n_remaining_clusters, n_assigned_clusters;

      std::vector<std::size_t> assigned_data_points{data_idxs_[0]};

      std::vector<ArrayType> cluster_distances{n_clusters_};
      std::size_t            assigned_cluster = 0;

      std::vector<typename ArrayType::Type> weights(
          n_points_);  // weight for choosing each data point
      std::vector<typename ArrayType::Type> interval(
          n_points_);  // interval for defining random distribtion
      std::iota(std::begin(interval), std::end(interval), 0);  // fill interval with range

      for (std::size_t cur_cluster = 1; cur_cluster < n_clusters_; ++cur_cluster)
      {
        n_remaining_data_points = n_points_ - 1;
        n_remaining_clusters    = n_clusters_ - 1;
        n_assigned_data_points  = n_points_ - n_remaining_data_points;
        n_assigned_clusters     = n_clusters_ - n_remaining_clusters;

        // calculate distances to all clusters
        for (std::size_t i = 0; i < n_assigned_clusters; ++i)
        {
          for (std::size_t l = 0; l < n_points_; ++l)
          {
            for (std::size_t k = 0; k < n_dimensions_; ++k)
            {
              temp_k_.Set(l, k, k_means_.At(i, k));
            }
          }

          // overwrite the data points already assigned to clusters
          for (std::size_t l = 0; l < n_assigned_data_points; ++l)
          {
            for (std::size_t k = 0; k < n_dimensions_; ++k)
            {
              temp_k_.Set(assigned_data_points[l], k, 0);
            }
          }
          cluster_distances[i] = fetch::math::metrics::EuclideanDistance(data, temp_k_, 1);
        }

        // select smallest distance to cluster for each data point and square
        for (std::size_t m = 0; m < n_points_; ++m)
        {
          // ignore already assigned data points
          if (std::find(assigned_data_points.begin(), assigned_data_points.end(), m) ==
              assigned_data_points.end())
          {
            running_mean_ = std::numeric_limits<typename ArrayType::Type>::max();
            for (std::size_t i = 0; i < n_assigned_clusters; ++i)
            {
              if (cluster_distances[i][m] < running_mean_)
              {
                running_mean_    = cluster_distances[i][m];
                assigned_cluster = i;
              }
            }
            weights[m] = cluster_distances[assigned_cluster][m];
          }
          else
          {
            weights[m] = 0;
          }
        }
        fetch::math::Square(weights);

        // select point as new cluster centre
        std::piecewise_constant_distribution<typename ArrayType::Type> dist(
            std::begin(interval), std::end(interval), std::begin(weights));
        assigned_data_points.push_back(static_cast<std::size_t>(dist(rng_)));

        for (std::size_t j = 0; j < n_dimensions_; ++j)
        {
          k_means_.Set(cur_cluster, j, data.At(assigned_data_points.back(), j));
        }
      }
      break;
    }

    case InitMode::Forgy:
    {
      // Forgy initialisation - pick random data points as kmean centres
      for (std::size_t i = 0; i < n_clusters_; ++i)
      {
        for (std::size_t j = 0; j < n_dimensions_; ++j)
        {
          k_means_.Set(i, j, data.At(data_idxs_[i], j));
        }
      }
      break;
    }

    default:
      throw std::runtime_error("no such initialization mode for KMeans");
    }
  }

  /**
   * part 1 of the iterative process for KMeans:
   * Assign each data point to a cluster based on euclidean distance
   */
  void Assign(ArrayType const &data)
  {
    // replicate kmeans which is 1 x n_dims into n_data x n_dims
    // allows for easy call to EuclideanDistance
    for (std::size_t i = 0; i < n_clusters_; ++i)
    {
      for (std::size_t j = 0; j < n_points_; ++j)
      {
        for (std::size_t k = 0; k < n_dimensions_; ++k)
        {
          temp_k_.Set(j, k, k_means_.At(i, k));
        }
      }
      k_euclids_[i] = fetch::math::metrics::EuclideanDistance(data, temp_k_, 1);
    }

    // now we have a vector of n_data x 1 Arrays
    // we have to go through and compare which is smallest for each K and make the assignment
    std::fill(k_count_.begin(), k_count_.end(), 0);
    for (std::size_t i = 0; i < n_points_; ++i)
    {
      running_mean_ = std::numeric_limits<double>::max();
      for (std::size_t j = 0; j < n_clusters_; ++j)
      {
        if (k_euclids_[j][i] < running_mean_)
        {
          running_mean_ = k_euclids_[j][i];
          assigned_k_   = j;
        }
      }
      k_assignment_.Set(i, 0, static_cast<typename ArrayType::Type>(assigned_k_));
      ++k_count_[assigned_k_];
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
    reassign_ = false;
    std::fill(empty_clusters_.begin(), empty_clusters_.end(), 0);
    for (std::size_t i = 0; i < n_clusters_; ++i)
    {
      if (k_count_[i] == 0)
      {
        empty_clusters_[i] = 1;
        reassign_          = true;
      }
    }

    // if a category has been completely eliminated! we should randomly assign one data point to it
    if (reassign_)
    {
      std::fill(reassigned_k_.begin(), reassigned_k_.end(), 0);
      std::shuffle(data_idxs_.begin(), data_idxs_.end(), rng_);

      for (std::size_t i = 0; i < n_clusters_; ++i)
      {
        if (empty_clusters_[i] == 1)
        {
          reassigned_k_[i] = static_cast<std::size_t>(k_assignment_[i]);
          k_assignment_[i] = static_cast<typename ArrayType::Type>(data_idxs_[i]);
          ++k_count_[i];
        }
      }
    }
  }

  /**
   * Method reverts reassigned data because convergence is complete
   */
  void UnReassign()
  {
    if (reassign_)
    {
      for (std::size_t i = 0; i < n_clusters_; ++i)
      {
        if (empty_clusters_[i] == 1)
        {
          k_assignment_[i] = static_cast<typename ArrayType::Type>(reassigned_k_[i]);
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
    std::fill(k_means_.begin(), k_means_.end(), 0);
    // get KSums
    for (std::size_t i = 0; i < n_points_; ++i)
    {
      auto cur_k = static_cast<std::size_t>(k_assignment_[i]);
      for (std::size_t j = 0; j < n_dimensions_; ++j)
      {
        k_means_.Set(cur_k, j, k_means_.At(cur_k, j) + data.At(i, j));
      }
    }

    // divide sums to get KMeans
    for (std::size_t m = 0; m < n_clusters_; ++m)
    {
      for (std::size_t i = 0; i < n_dimensions_; ++i)
      {
        k_means_.Set(m, i, k_means_.At(m, i) / static_cast<typename ArrayType::Type>(k_count_[m]));
      }
    }
  }

  /**
   * checks for convergence of iterative algorithm
   */
  bool NotConverged()
  {
    // completed max n loops
    if (loop_counter_ >= max_loops_)
    {
      return false;
    }
    ++loop_counter_;

    // last update changed nothing (i.e converged)
    if (k_assignment_ == prev_k_assignment_)
    {
      ++no_change_count_;
    }
    else
    {
      no_change_count_ = 0;
    }

    if (no_change_count_ >= max_no_change_convergence_)
    {
      return false;
    }
    prev_k_assignment_.Copy(k_assignment_);
    prev_k_means_.Copy(k_means_);

    return true;
  }

  std::size_t n_points_;
  std::size_t n_dimensions_;
  std::size_t n_clusters_;

  double running_mean_;  // we'll use this find the smallest euclidean distance out of K comparisons

  std::default_random_engine rng_;

  std::vector<std::size_t> data_idxs_;       // a vector of indices to the data used for shuffling
  std::vector<std::size_t> empty_clusters_;  // a vector tracking whenever a cluster goes empty

  ArrayType k_means_;       // current cluster centres
  ArrayType prev_k_means_;  // previous cluster centres (for checking convergence)
  ArrayType temp_k_;        // a container for ease of access to using Euclidean function

  ArrayType k_assignment_;       // current data to cluster assignment
  ArrayType prev_k_assignment_;  // previous data to cluster assignment (for checkign convergence)
  std::vector<std::size_t> reassigned_k_;  // reassigned data to cluster assignment

  std::vector<std::size_t>
                         k_count_;  // count of how many data points per cluster (for checking reassignment)
  std::vector<ArrayType> k_euclids_;  // container for current euclid distances

  std::size_t assigned_k_;  // current cluster to assign

  std::size_t no_change_count_ = 0;  // number of times there was no change in k_assignment in a row
  std::size_t max_no_change_convergence_;  // max number of times k_assignment can not change before
  // convergence

  bool reassign_;

  std::size_t loop_counter_ = 0;
  std::size_t max_loops_;

  enum InitMode
  {
    KMeansPP = 0,
    Forgy    = 1
  };
  std::size_t init_mode_;
};

}  // namespace details

template <typename ArrayType>
ArrayType KMeans(ArrayType const &data, std::size_t const &K, std::size_t const &r_seed,
                 std::size_t max_loops = 100)
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
      ret[i] = static_cast<typename ArrayType::Type>(i);
    }
  }
  else  // real work happens in these cases
  {
    details::KMeansImplementation<ArrayType>(data, K, ret, r_seed, max_loops);
  }

  return ret;
}

}  // namespace clustering
}  // namespace math
}  // namespace fetch
