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

#include "core/vector.hpp"
#include "math/free_functions/free_functions.hpp"
#include "math/free_functions/metrics/metrics.hpp"
#include "math/meta/type_traits.hpp"
#include "random"

#include <set>

/**
 * assigns the absolute of x to this array
 * @param x
 */

namespace fetch {
namespace math {
namespace clustering {

enum class InitMode
{
  KMeansPP = 0,  // kmeans++, a good default choice
  Forgy    = 1,  // Forgy, randomly initialize clusters to data points
  PrevK    = 2   // PrevK, use previous k_assignment to determine cluster centres
};
enum class KInferenceMode
{
  Off            = 0,
  NClusters      = 1,  // infer K by counting number of previously assigned clusters
  HighestCluster = 2   // infer K by using highest valued previous cluster assignment
};

namespace details {

template <typename ArrayType>
class KMeansImplementation
{
public:
  KMeansImplementation(ArrayType const &data, std::size_t const &n_clusters, ArrayType &ret,
                       std::size_t const &r_seed, std::size_t const &max_loops, InitMode init_mode,
                       std::size_t max_no_change_convergence)
    : n_clusters_(n_clusters)
    , max_no_change_convergence_(std::move(max_no_change_convergence))
    , max_loops_(max_loops)
    , init_mode_(init_mode)
  {

    n_points_     = data.shape()[0];
    n_dimensions_ = data.shape()[1];

    fetch::core::Vector<std::size_t> k_assignment_shape{n_points_, 1};
    k_assignment_ = ArrayType(k_assignment_shape);

    KMeansSetup(data, r_seed);

    ComputeKMeans(data, ret);

    // assign the final output
    ret = k_assignment_;
  }

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
                       ArrayType k_assignment, std::size_t max_no_change_convergence)
    : n_clusters_(n_clusters)
    , max_no_change_convergence_(std::move(max_no_change_convergence))
    , max_loops_(max_loops)
    , k_assignment_(std::move(k_assignment))
  {

    // seed random number generator
    rng_.seed(uint32_t(r_seed));

    n_points_     = data.shape()[0];
    n_dimensions_ = data.shape()[1];

    init_mode_ = InitMode::PrevK;  // since prev_k_assignment is specified, the initialization will
                                   // be to use that

    KMeansSetup(data, r_seed);

    ComputeKMeans(data, ret);

    // in some cases, we need to remap cluster labels back to input previous labelling scheme
    if ((init_mode_ == InitMode::PrevK) && (k_inference_mode_ == KInferenceMode::NClusters))
    {
      ReMapClusters();
    }

    // assign the final output
    ret = k_assignment_;
  }

  KMeansImplementation(ArrayType const &data, ArrayType &ret, std::size_t const &r_seed,
                       std::size_t const &max_loops, ArrayType k_assignment,
                       std::size_t           max_no_change_convergence,
                       KInferenceMode const &k_inference_mode)
    : max_no_change_convergence_(max_no_change_convergence)
    , max_loops_(max_loops)
    , k_assignment_(std::move(k_assignment))
    , k_inference_mode_(k_inference_mode)
  {

    // seed random number generator
    rng_.seed(uint32_t(r_seed));

    n_points_     = data.shape()[0];
    n_dimensions_ = data.shape()[1];

    init_mode_ = InitMode::PrevK;  // since prev_k_assignment is specified, the initialization will
    // be to use that

    KMeansSetup(data, r_seed);

    ComputeKMeans(data, ret);

    // in some cases, we need to remap cluster labels back to input previous labelling scheme
    if ((init_mode_ == InitMode::PrevK) && (k_inference_mode_ == KInferenceMode::NClusters))
    {
      ReMapClusters();
    }

    // assign the final output
    ret = k_assignment_;
  }

  /**
   * The remainder of initialization that is common to all constructors is carried out here
   */
  void KMeansSetup(ArrayType const &data, std::size_t const &r_seed)
  {
    // seed random number generator
    rng_.seed(uint32_t(r_seed));
    loop_counter_ = 0;

    temp_k_ = ArrayType(data.shape());

    // instantiate counter with zeros
    InitialiseKMeans(data);

    // initialise assignment
    fetch::core::Vector<std::size_t> k_assignment_shape{n_points_, 1};
    prev_k_assignment_ = ArrayType(
        k_assignment_shape);  // need to keep a record of previous to check for convergence
    for (std::size_t l = 0; l < prev_k_assignment_.size(); ++l)
    {
      prev_k_assignment_.Set(l, -1);
    }
    reassigned_k_ =
        fetch::core::Vector<int>(n_points_, -1);  // technically this limits us to fewer groups

    // initialise size of euclidean distance container
    k_euclids_      = fetch::core::Vector<ArrayType>(n_clusters_);
    empty_clusters_ = fetch::core::Vector<std::size_t>(n_clusters_);
  };

private:
  /**
   * The main iterative loop that calculates KMeans clustering
   */
  void ComputeKMeans(ArrayType const &data, ArrayType &ret)
  {
    while (NotConverged())
    {
      Assign(data);
      Update(data);
    }
    UnReassign();
  }

  /**
   * kmeans cluster centre initialisation
   * This is very important for defining the behaviour of the clustering algorithm
   * @param data
   */
  void InitialiseKMeans(ArrayType const &data)
  {
    data_idxs_ = fetch::core::Vector<std::size_t>(n_points_);
    if (k_inference_mode_ == KInferenceMode::Off)
    {
      k_count_ = fetch::core::Vector<std::size_t>(n_clusters_, 0);

      // shuffle the data
      std::iota(std::begin(data_idxs_), std::end(data_idxs_), 0);
      std::shuffle(data_idxs_.begin(), data_idxs_.end(), rng_);

      if (n_clusters_ != 0)
      {
        // initialise k means
        fetch::core::Vector<std::size_t> k_means_shape{n_clusters_, n_dimensions_};
        k_means_      = ArrayType(k_means_shape);
        prev_k_means_ = ArrayType::Zeroes(k_means_shape);
      }
    }

    switch (init_mode_)
    {

    case InitMode::PrevK:
    {
      assert(k_assignment_.shape()[0] == n_points_);
      assert(k_assignment_.size() == n_points_);

      // check if we know enough to initialise from previous clusters
      bool sufficient_previous_assignment;
      if (k_inference_mode_ != KInferenceMode::Off)
      {
        InferK(sufficient_previous_assignment);
      }
      else
      {

        // if user has set a specific value for K then we must have non-zero count for every cluster
        std::fill(k_count_.begin(), k_count_.end(), 0);
        for (std::size_t j = 0; j < n_points_; ++j)
        {
          if (!(k_assignment_.At(j, 0) < 0))  // previous unassigned data points must be assigned
                                              // with a negative cluster value
          {
            k_count_[static_cast<std::size_t>(k_assignment_.At(j, 0))] += 1;
          }
        }

        sufficient_previous_assignment = true;
        for (std::size_t j = 0; sufficient_previous_assignment && j < n_clusters_; ++j)
        {
          if (k_count_[j] == 0)
          {
            sufficient_previous_assignment = false;
          }
        }
        n_clusters_ = k_count_.size();
      }

      // initialise k means
      fetch::core::Vector<std::size_t> k_means_shape{n_clusters_, n_dimensions_};
      k_means_      = ArrayType(k_means_shape);
      prev_k_means_ = ArrayType::Zeroes(k_means_shape);

      // if any clusters begin empty, we'll actually default to KMeans++ via fall through
      if (sufficient_previous_assignment)
      {
        PartialUpdate(data);
      }
      else
      {
        KMeansPPInitialisation(data);
      }
      break;
    }
    case InitMode::KMeansPP:
    {
      KMeansPPInitialisation(data);
      break;
    }
    case InitMode::Forgy:
    {
      ForgyInitialisation(data);
      break;
    }
    default:
      throw std::runtime_error("no such initialization mode for KMeans");
    }

    // reset the kcount
    std::fill(k_count_.begin(), k_count_.end(), 0);  // reset the k_count
  }

  /**
   * Infers the value of K based on previously assigned data points to clusters
   * @param sufficient_previous_assignment boolean indicating whether we have enough previous
   * assignment to initialise this way
   */
  void InferK(bool &sufficient_previous_assignment)
  {
    assert(k_inference_mode_ != KInferenceMode::Off);
    assert(k_count_.size() == 0);

    if (k_inference_mode_ == KInferenceMode::HighestCluster)
    {
      // infer K to be equal to the value of the highest numbered cluster encountered in the
      // assignment

      for (std::size_t j = 0; j < n_points_; ++j)
      {
        // previous unassigned data points must be assigned with a negative cluster value
        if (!(k_assignment_.At(j, 0) < 0))
        {
          while (k_count_.size() <= static_cast<std::size_t>(k_assignment_.At(j, 0)))
          {
            k_count_.emplace_back(0);
          }
          ++k_count_[static_cast<std::size_t>(k_assignment_.At(j, 0))];
        }
      }
      n_clusters_ = k_count_.size();
    }
    else if (k_inference_mode_ == KInferenceMode::NClusters)
    {
      //////////////////////////////////////////////////////////////////////////////
      /// infer K to be equal to the number of unique clusters assigned at input ///
      //////////////////////////////////////////////////////////////////////////////

      // get the set of existing clusters and the count of data points assigned to each cluster
      std::unordered_map<int, std::size_t>         prev_cluster_count{};
      std::set<int>                                previous_cluster_labels{};
      std::unordered_map<std::size_t, std::size_t> reverse_cluster_assignment_map{};

      // default value not used
      int current_cluster_label = std::numeric_limits<int>::max();

      // get the set of input labels, get the count for each label
      for (std::size_t j = 0; j < n_points_; ++j)
      {
        // get the label and add to set if new
        current_cluster_label = static_cast<int>(k_assignment_.At(j, 0));
        if (previous_cluster_labels.find(current_cluster_label) == previous_cluster_labels.end())
        {
          previous_cluster_labels.insert(current_cluster_label);
        }

        // if this data point is assigned
        if (current_cluster_label >= 0)
        {
          // if this cluster already exists in the map
          if (prev_cluster_count.find(current_cluster_label) != prev_cluster_count.end())
          {
            ++(prev_cluster_count.find(current_cluster_label)->second);
          }
          else
          {
            prev_cluster_count.insert(std::pair<int, std::size_t>(current_cluster_label, 1));
          }
        }
      }

      // store the count per cluster, and map
      std::size_t cluster_count = 0;
      for (auto const &pc_label : previous_cluster_labels)
      {
        if (pc_label >= 0)  // ignore previous negative labels - these indicate unassigned clusters
        {
          k_count_.emplace_back(prev_cluster_count.find(pc_label)->second);

          // keep a map of internal cluster labels to input cluster labels
          cluster_assignment_map_.insert(std::pair<std::size_t, std::size_t>(
              cluster_count, prev_cluster_count.find(pc_label)->first));
          reverse_cluster_assignment_map.insert(std::pair<std::size_t, std::size_t>(
              prev_cluster_count.find(pc_label)->first, cluster_count));
          ++cluster_count;
        }
      }
      n_clusters_ = cluster_count;

      // overwrite input assignments with internal labelling scheme
      for (std::size_t j = 0; j < n_points_; ++j)
      {
        current_cluster_label = static_cast<int>(k_assignment_.At(j, 0));

        if (current_cluster_label >= 0)
        {
          k_assignment_.Set(j, 0,
                            static_cast<typename ArrayType::Type>(
                                reverse_cluster_assignment_map
                                    .find(static_cast<std::size_t>(k_assignment_.At(j, 0)))
                                    ->second));
        }
      }
    }

    // failing this assertion generally implies that there are fewer than 2 separate
    // cluster labels for the previously assigned data - this is not permissble since
    // it makes inference impossible
    assert(n_clusters_ > 1);

    // if user wants us to figure out how many clusters to set then we only need to find a
    // single case with non-zero count
    sufficient_previous_assignment = false;
    for (std::size_t j = 0; (!sufficient_previous_assignment) && j < n_clusters_; ++j)
    {
      if (k_count_[j] != 0)
      {
        sufficient_previous_assignment = true;
      }
    }
  }

  /**
   * Forgy Initialisation - i.e. clusters are set as random data points
   * @param data
   */
  void ForgyInitialisation(ArrayType const &data)
  {
    // Forgy initialisation - pick random data points as kmean centres
    for (std::size_t i = 0; i < n_clusters_; ++i)
    {
      for (std::size_t j = 0; j < n_dimensions_; ++j)
      {
        k_means_.Set(i, j, data.At(data_idxs_[i], j));
      }
    }
  }

  /**
   * KMeans++ initialisation
   * @param data
   */
  void KMeansPPInitialisation(ArrayType const &data)
  {
    // assign first cluster centre
    for (std::size_t j = 0; j < n_dimensions_; ++j)
    {
      k_means_.Set(0, j, data.At(data_idxs_[0], j));
    }

    // assign remaining cluster centres
    std::size_t n_remaining_data_points = n_points_ - 1;
    std::size_t n_remaining_clusters    = n_clusters_ - 1;

    fetch::core::Vector<std::size_t> assigned_data_points{data_idxs_[0]};

    fetch::core::Vector<ArrayType> cluster_distances(n_clusters_);
    std::size_t                    assigned_cluster = 0;

    fetch::core::Vector<typename ArrayType::Type> weights(
        n_points_);  // weight for choosing each data point
    fetch::core::Vector<typename ArrayType::Type> interval(
        n_points_);  // interval for defining random distribtion
    std::iota(std::begin(interval), std::end(interval), 0);  // fill interval with range

    for (std::size_t cur_cluster = 1; cur_cluster < n_clusters_; ++cur_cluster)
    {
      // calculate distances to all clusters
      for (std::size_t i = 0; i < (n_clusters_ - n_remaining_clusters); ++i)
      {
        for (std::size_t l = 0; l < n_points_; ++l)
        {
          for (std::size_t k = 0; k < n_dimensions_; ++k)
          {
            temp_k_.Set(l, k, k_means_.At(i, k));
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
          for (std::size_t i = 0; i < (n_clusters_ - n_remaining_clusters); ++i)
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

      auto val      = dist(rng_);
      auto tmp_rand = static_cast<std::size_t>(val);

      assert((tmp_rand < n_points_) && (tmp_rand >= 0));

      assigned_data_points.push_back(tmp_rand);

      for (std::size_t j = 0; j < n_dimensions_; ++j)
      {
        k_means_.Set(cur_cluster, j, data.At(assigned_data_points.back(), j));
      }

      // update count of remaining data points and clusters
      --n_remaining_data_points;
      --n_remaining_clusters;
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
      running_mean_ = std::numeric_limits<typename ArrayType::Type>::max();
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
      std::fill(reassigned_k_.begin(), reassigned_k_.end(), -1);
      std::shuffle(data_idxs_.begin(), data_idxs_.end(), rng_);

      for (std::size_t i = 0; i < n_clusters_; ++i)
      {
        if (empty_clusters_[i] == 1)
        {
          reassigned_k_[data_idxs_[i]] = static_cast<int>(k_assignment_[i]);
          k_assignment_[data_idxs_[i]] = static_cast<typename ArrayType::Type>(i);
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
          k_assignment_[i] = static_cast<typename ArrayType::Type>(reassigned_k_[data_idxs_[i]]);
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
    std::size_t cur_k;
    for (std::size_t i = 0; i < n_points_; ++i)
    {
      cur_k = static_cast<std::size_t>(k_assignment_[i]);
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
   * Calculate kMeans cluster centres under the assumption that there may be some data points not
   * yet assigned
   */
  void PartialUpdate(ArrayType const &data)
  {
    std::fill(k_means_.begin(), k_means_.end(), 0);
    // get KSums
    std::size_t cur_k;
    for (std::size_t i = 0; i < n_points_; ++i)
    {
      if (!(k_assignment_[i] < 0))  // ignore if no assignment made yet
      {
        cur_k = static_cast<std::size_t>(k_assignment_[i]);
        for (std::size_t j = 0; j < n_dimensions_; ++j)
        {
          k_means_.Set(cur_k, j, k_means_.At(cur_k, j) + data.At(i, j));
        }
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

  void ReMapClusters()
  {
    for (std::size_t i = 0; i < n_points_; ++i)
    {
      assert(cluster_assignment_map_.find(static_cast<std::size_t>(k_assignment_.At(i, 0))) !=
             cluster_assignment_map_.end());
      // overwrite every clust assignment with its equivalent previous label at input
      k_assignment_.Set(
          i, 0,
          static_cast<typename ArrayType::Type>(
              cluster_assignment_map_.find(static_cast<std::size_t>(k_assignment_.At(i, 0)))
                  ->second));
    }
  }

  static constexpr std::size_t INVALID       = std::numeric_limits<std::size_t>::max();
  std::size_t                  n_points_     = INVALID;
  std::size_t                  n_dimensions_ = INVALID;
  std::size_t                  n_clusters_   = INVALID;

  std::size_t no_change_count_ = INVALID;  // times there was no change in k_assignment in a row
  std::size_t max_no_change_convergence_ =
      INVALID;  // max no change k_assignment before convergence
  std::size_t loop_counter_ = INVALID;
  std::size_t max_loops_    = INVALID;
  std::size_t assigned_k_   = INVALID;  // current cluster to assign

  // used to find the smallest distance out of K comparisons
  typename ArrayType::Type running_mean_ = std::numeric_limits<typename ArrayType::Type>::max();

  std::default_random_engine rng_;

  fetch::core::Vector<std::size_t>
      data_idxs_;  // a vector of indices to the data used for shuffling
  fetch::core::Vector<std::size_t>
      empty_clusters_;  // a vector tracking whenever a cluster goes empty

  ArrayType k_means_;       // current cluster centres
  ArrayType prev_k_means_;  // previous cluster centres (for checking convergence)
  ArrayType temp_k_;        // a container for ease of access to using Euclidean function

  ArrayType k_assignment_;       // current data to cluster assignment
  ArrayType prev_k_assignment_;  // previous data to cluster assignment (for checkign convergence)
  fetch::core::Vector<int> reassigned_k_;  // reassigned data to cluster assignment

  fetch::core::Vector<std::size_t> k_count_;  // count of how many data points assigned per cluster
  fetch::core::Vector<ArrayType>   k_euclids_;  // container for current euclid distances

  // map previously assigned clusters to current clusters
  std::unordered_map<std::size_t, std::size_t>
      cluster_assignment_map_{};  // <internal label, original label>

  bool reassign_;

  InitMode       init_mode_        = InitMode::KMeansPP;
  KInferenceMode k_inference_mode_ = KInferenceMode::Off;
};

}  // namespace details

/**
 * Interface to KMeans algorithm
 * @tparam ArrayType    fetch library Array type
 * @param data          input data to cluster in format n_data x n_dims
 * @param K             number of clusters
 * @param r_seed        random seed
 * @param max_loops     maximum loops until convergence assumed
 * @return              ArrayType of format n_data x 1 with values indicating cluster
 */
template <typename ArrayType>
ArrayType KMeans(ArrayType const &data, std::size_t const &r_seed, std::size_t const &K,
                 std::size_t max_loops = 100, InitMode init_mode = InitMode::KMeansPP,
                 std::size_t max_no_change_convergence = 10)
{
  std::size_t n_points = data.shape()[0];

  // we can't have more clusters than data points
  assert(K <= n_points);  // you can't have more clusters than data points
  assert(K > 1);          // why would you run k means clustering with only one cluster?

  fetch::core::Vector<std::size_t> ret_array_shape{n_points, 1};
  ArrayType                        ret{ret_array_shape};

  if (n_points == K)  // very easy to cluster!
  {
    for (std::size_t i = 0; i < n_points; ++i)
    {
      ret[i] = static_cast<typename ArrayType::Type>(i);
    }
  }
  else  // real work happens in these cases
  {
    details::KMeansImplementation<ArrayType>(data, K, ret, r_seed, max_loops, init_mode,
                                             max_no_change_convergence);
  }

  return ret;
}

/**
 * Interface to KMeans algorithm
 * @tparam ArrayType        fetch library Array type
 * @param data              input data to cluster in format n_data x n_dims
 * @param r_seed            random seed
 * @param prev_assignment   previous K assignment on which to initialise
 * @param k_inference_mode  the method for defining how to infer K
 * @param max_loops         maximum loops until convergence assumed
 * @param max_no_change_convergence     number of iterations with no change to assignment that
 * counts as convergence
 * @return                  ArrayType of format n_data x 1 with values indicating cluster
 */
template <typename ArrayType>
ArrayType KMeans(ArrayType const &data, std::size_t const &r_seed, ArrayType const &prev_assignment,
                 KInferenceMode const &k_inference_mode, std::size_t max_loops = 100,
                 std::size_t max_no_change_convergence = 10)
{
  std::size_t                      n_points = data.shape()[0];
  fetch::core::Vector<std::size_t> ret_array_shape{n_points, 1};
  ArrayType                        ret{ret_array_shape};
  details::KMeansImplementation<ArrayType>(data, ret, r_seed, max_loops, prev_assignment,
                                           max_no_change_convergence, k_inference_mode);

  return ret;
}

/**
 * Interface to KMeans algorithm
 * @tparam ArrayType        fetch library Array type
 * @param data              input data to cluster in format n_data x n_dims
 * @param r_seed            random seed
 * @param K                 number of clusters
 * @param prev_assignment   previous K assignment on which to initialise
 * @param max_loops         maximum loops until convergence assumed
 * @param max_no_change_convergence     number of iterations with no change to assignment that
 * counts as convergence
 * @return                  ArrayType of format n_data x 1 with values indicating cluster
 */
template <typename ArrayType>
ArrayType KMeans(ArrayType const &data, std::size_t const &r_seed, std::size_t const &K,
                 ArrayType const &prev_assignment, std::size_t max_loops = 100,
                 std::size_t max_no_change_convergence = 10)
{
  std::size_t                      n_points = data.shape()[0];
  fetch::core::Vector<std::size_t> ret_array_shape{n_points, 1};
  ArrayType                        ret{ret_array_shape};

  assert(K <= n_points);  // you can't have more clusters than data points
  assert(K != 1);         // why would you run k means clustering with only one cluster?

  if (n_points == K)  // very easy to cluster!
  {
    for (std::size_t i = 0; i < n_points; ++i)
    {
      ret[i] = static_cast<typename ArrayType::Type>(i);
    }
  }
  else  // real work happens in these cases
  {
    details::KMeansImplementation<ArrayType>(data, K, ret, r_seed, max_loops, prev_assignment,
                                             max_no_change_convergence);
  }
  return ret;
}

}  // namespace clustering
}  // namespace math
}  // namespace fetch
