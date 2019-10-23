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

#include "core/random.hpp"
#include "core/vector.hpp"
#include "math/distance/euclidean.hpp"
#include "math/standard_functions/pow.hpp"
#include "math/tensor.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fetch {
namespace math {
namespace clustering {

using ClusteringType = Tensor<int64_t>;

enum class InitMode
{
  KMeansPP = 0,  // kmeans++, a good default choice
  Forgy    = 1,  // Forgy, randomly initialise clusters to data points
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

  using DataType        = typename ArrayType::Type;
  using SizeType        = typename ArrayType::SizeType;
  using ArrayOfSizeType = typename fetch::math::Tensor<SizeType>;

public:
  KMeansImplementation(ArrayType const &data, SizeType const &n_clusters, ClusteringType &ret,
                       SizeType const &r_seed, SizeType const &max_loops, InitMode init_mode,
                       SizeType max_no_change_convergence)
    : n_clusters_(n_clusters)
    , max_no_change_convergence_(std::move(max_no_change_convergence))
    , max_loops_(max_loops)
    , init_mode_(init_mode)
  {

    n_points_     = data.shape()[0];
    n_dimensions_ = data.shape()[1];

    k_assignment_ = ClusteringType(n_points_);

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
   * @param init_mode what type of initialisation to use
   */
  KMeansImplementation(ArrayType const &data, SizeType const &n_clusters, ClusteringType &ret,
                       SizeType const &r_seed, SizeType const &max_loops,
                       ClusteringType k_assignment, SizeType max_no_change_convergence)
    : n_clusters_(n_clusters)
    , max_no_change_convergence_(std::move(max_no_change_convergence))
    , max_loops_(max_loops)
    , k_assignment_(std::move(k_assignment))
  {

    n_points_     = data.shape()[0];
    n_dimensions_ = data.shape()[1];

    init_mode_ = InitMode::PrevK;  // since prev_k_assignment is specified, the initialisation will
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

  KMeansImplementation(ArrayType const &data, ClusteringType &ret, SizeType const &r_seed,
                       SizeType const &max_loops, ClusteringType k_assignment,
                       SizeType max_no_change_convergence, KInferenceMode const &k_inference_mode)
    : max_no_change_convergence_(max_no_change_convergence)
    , max_loops_(max_loops)
    , k_assignment_(std::move(k_assignment))
    , k_inference_mode_(k_inference_mode)
  {

    n_points_     = data.shape()[0];
    n_dimensions_ = data.shape()[1];

    init_mode_ = InitMode::PrevK;  // since prev_k_assignment is specified, the initialisation will
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
   * The remainder of initialisation that is common to all constructors is carried out here
   */
  void KMeansSetup(ArrayType const &data, SizeType const &r_seed)
  {
    // seed random number generator
    lfg_.Seed(uint32_t(r_seed));
    loop_counter_ = 0;

    temp_k_ = ArrayType(data.shape());

    // instantiate counter with zeros
    InitialiseKMeans(data);

    // initialise assignment
    prev_k_assignment_ =
        ClusteringType(n_points_);  // need to keep a record of previous to check for convergence
    for (SizeType l = 0; l < prev_k_assignment_.size(); ++l)
    {
      prev_k_assignment_.Set(l, -1);
    }

    reassigned_k_ = ClusteringType(n_points_);
    for (SizeType j = 0; j < reassigned_k_.size(); ++j)
    {
      reassigned_k_.Set(j, -1);
    }

    // initialise size of euclidean distance container
    k_euclids_      = fetch::core::Vector<ArrayType>(n_clusters_);
    empty_clusters_ = fetch::core::Vector<SizeType>(n_clusters_);
  };

private:
  /**
   * The main iterative loop that calculates KMeans clustering
   */
  void ComputeKMeans(ArrayType const &data, ClusteringType & /*ret*/)
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
    data_idxs_ = fetch::core::Vector<SizeType>(n_points_);
    if (k_inference_mode_ == KInferenceMode::Off)
    {
      k_count_ = fetch::core::Vector<SizeType>(n_clusters_, 0);

      // shuffle the data
      std::iota(std::begin(data_idxs_), std::end(data_idxs_), 0);
      fetch::random::Shuffle(lfg_, data_idxs_, data_idxs_);

      if (n_clusters_ != 0)
      {
        // initialise k means
        k_means_      = ArrayType({n_clusters_, n_dimensions_});
        prev_k_means_ = ArrayType({n_clusters_, n_dimensions_});
      }
    }

    switch (init_mode_)
    {

    case InitMode::PrevK:
    {
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
        for (SizeType j = 0; j < n_points_; ++j)
        {
          if (!(k_assignment_.At(j) < 0))  // previous unassigned data points must be assigned
                                           // with a negative cluster value
          {
            k_count_[static_cast<SizeType>(k_assignment_.At(j))] += 1;
          }
        }

        sufficient_previous_assignment = true;
        for (SizeType j = 0; sufficient_previous_assignment && j < n_clusters_; ++j)
        {
          if (k_count_[j] == 0)
          {
            sufficient_previous_assignment = false;
          }
        }
        n_clusters_ = k_count_.size();
      }

      // initialise k means
      k_means_      = ArrayType({n_clusters_, n_dimensions_});
      prev_k_means_ = ArrayType({n_clusters_, n_dimensions_});

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
      throw exceptions::InvalidMode("no such initialisation mode for KMeans");
    }

    // reset the kcount
    std::fill(k_count_.begin(), k_count_.end(), 0);
  }

  /**
   * Infers the value of K based on previously assigned data points to clusters
   * @param sufficient_previous_assignment boolean indicating whether we have enough previous
   * assignment to initialise this way
   */
  void InferK(bool &sufficient_previous_assignment)
  {
    assert(k_inference_mode_ != KInferenceMode::Off);
    assert(k_count_.empty());

    if (k_inference_mode_ == KInferenceMode::HighestCluster)
    {
      // infer K to be equal to the value of the highest numbered cluster encountered in the
      // assignment

      for (SizeType j = 0; j < n_points_; ++j)
      {
        // previous unassigned data points must be assigned with a negative cluster value
        if (!(k_assignment_.At(j) < 0))
        {
          while (k_count_.size() <= static_cast<SizeType>(k_assignment_.At(j)))
          {
            k_count_.emplace_back(0);
          }
          ++k_count_[static_cast<SizeType>(k_assignment_.At(j))];
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
      std::unordered_map<DataType, SizeType> prev_cluster_count{};
      std::set<DataType>                     previous_cluster_labels{};
      std::unordered_map<SizeType, SizeType> reverse_cluster_assignment_map{};

      // default value not used
      DataType current_cluster_label = numeric_max<DataType>();

      // get the set of input labels, get the count for each label
      for (SizeType j = 0; j < n_points_; ++j)
      {
        // get the label and add to set if new
        current_cluster_label = static_cast<DataType>(k_assignment_.At(j));
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
            prev_cluster_count.insert(std::pair<DataType, SizeType>(current_cluster_label, 1));
          }
        }
      }

      // store the count per cluster, and map
      SizeType cluster_count = 0;
      for (auto const &pc_label : previous_cluster_labels)
      {
        if (pc_label >= 0)  // ignore previous negative labels - these indicate unassigned clusters
        {
          k_count_.emplace_back(prev_cluster_count.find(pc_label)->second);

          // keep a map of internal cluster labels to input cluster labels
          cluster_assignment_map_.insert(std::pair<SizeType, SizeType>(
              cluster_count, prev_cluster_count.find(pc_label)->first));
          reverse_cluster_assignment_map.insert(std::pair<SizeType, SizeType>(
              prev_cluster_count.find(pc_label)->first, cluster_count));
          ++cluster_count;
        }
      }
      n_clusters_ = cluster_count;

      // overwrite input assignments with internal labelling scheme
      for (SizeType j = 0; j < n_points_; ++j)
      {
        current_cluster_label = k_assignment_.At(j);

        if (current_cluster_label >= 0)
        {
          k_assignment_.Set(
              j, static_cast<DataType>(reverse_cluster_assignment_map
                                           .find(static_cast<SizeType>(k_assignment_.At(j)))
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
    for (SizeType j = 0; (!sufficient_previous_assignment) && j < n_clusters_; ++j)
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
    for (SizeType i = 0; i < n_clusters_; ++i)
    {
      for (SizeType j = 0; j < n_dimensions_; ++j)
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
    for (SizeType j = 0; j < n_dimensions_; ++j)
    {
      k_means_.Set(0, j, data.At(data_idxs_[0], j));
    }

    // assign remaining cluster centres
    SizeType n_remaining_data_points = n_points_ - 1;
    SizeType n_remaining_clusters    = n_clusters_ - 1;

    fetch::core::Vector<SizeType> assigned_data_points{data_idxs_[0]};

    fetch::core::Vector<ArrayType> cluster_distances(n_clusters_);
    SizeType                       assigned_cluster = 0;

    fetch::core::Vector<DataType> weights(n_points_);   // weight for choosing each data point
    fetch::core::Vector<DataType> interval(n_points_);  // interval for defining random distribution
    std::iota(std::begin(interval), std::end(interval), 0);  // fill interval with range

    for (SizeType cur_cluster = 1; cur_cluster < n_clusters_; ++cur_cluster)
    {
      // calculate distances to all clusters
      for (SizeType i = 0; i < (n_clusters_ - n_remaining_clusters); ++i)
      {
        for (SizeType l = 0; l < n_points_; ++l)
        {
          for (SizeType k = 0; k < n_dimensions_; ++k)
          {
            temp_k_.Set(l, k, k_means_.At(i, k));
          }
        }

        cluster_distances.at(i) = fetch::math::distance::EuclideanMatrix(data, temp_k_, 1);
      }

      // select smallest distance to cluster for each data point and square
      for (SizeType m = 0; m < n_points_; ++m)
      {
        // ignore already assigned data points
        if (std::find(assigned_data_points.begin(), assigned_data_points.end(), m) ==
            assigned_data_points.end())
        {
          running_mean_ = numeric_max<DataType>();
          for (SizeType i = 0; i < (n_clusters_ - n_remaining_clusters); ++i)
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
      for (auto &val : weights)
      {
        fetch::math::Square(val, val);
      }

      // select point as new cluster centre
      std::piecewise_constant_distribution<double> dist(std::begin(interval), std::end(interval),
                                                        std::begin(weights));

      auto val      = dist(lfg_);
      auto tmp_rand = static_cast<SizeType>(val);

      assert((tmp_rand < n_points_) && (tmp_rand >= 0));
      assigned_data_points.push_back(tmp_rand);

      for (SizeType j = 0; j < n_dimensions_; ++j)
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
    // allows for easy call to Euclidean
    for (SizeType i = 0; i < n_clusters_; ++i)
    {
      for (SizeType j = 0; j < n_points_; ++j)
      {
        for (SizeType k = 0; k < n_dimensions_; ++k)
        {
          temp_k_.Set(j, k, k_means_.At(i, k));
        }
      }
      k_euclids_[i] = fetch::math::distance::EuclideanMatrix(data, temp_k_, 1);
    }

    // now we have a vector of n_data x 1 Arrays
    // we have to go through and compare which is smallest for each K and make the assignment
    std::fill(k_count_.begin(), k_count_.end(), 0);

    for (SizeType i = 0; i < n_points_; ++i)
    {
      running_mean_ = numeric_max<DataType>();
      for (SizeType j = 0; j < n_clusters_; ++j)
      {
        if (k_euclids_[j][i] < running_mean_)
        {
          running_mean_ = k_euclids_[j][i];
          assigned_k_   = static_cast<DataType>(j);
        }
      }
      k_assignment_.Set(i, assigned_k_);
      ++k_count_[static_cast<SizeType>(assigned_k_)];
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
    for (SizeType i = 0; i < n_clusters_; ++i)
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
      for (SizeType j = 0; j < reassigned_k_.size(); ++j)
      {
        reassigned_k_.Set(j, -1);
      }
      fetch::random::Shuffle(lfg_, data_idxs_, data_idxs_);

      for (SizeType i = 0; i < n_clusters_; ++i)
      {
        if (empty_clusters_[i] == 1)
        {
          reassigned_k_[data_idxs_[i]] = k_assignment_[i];
          k_assignment_[data_idxs_[i]] = static_cast<DataType>(i);
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
      for (SizeType i = 0; i < n_clusters_; ++i)
      {
        if (empty_clusters_[i] == 1)
        {
          k_assignment_[i] = reassigned_k_[data_idxs_[i]];
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
    k_means_.Fill(0);

    // get KSums
    SizeType cur_k;
    for (SizeType i = 0; i < n_points_; ++i)
    {
      cur_k = static_cast<SizeType>(k_assignment_[i]);
      for (SizeType j = 0; j < n_dimensions_; ++j)
      {
        k_means_.Set(cur_k, j, k_means_.At(cur_k, j) + data.At(i, j));
      }
    }

    // divide sums to get KMeans
    for (SizeType m = 0; m < n_clusters_; ++m)
    {
      for (SizeType i = 0; i < n_dimensions_; ++i)
      {
        k_means_.Set(m, i, k_means_.At(m, i) / static_cast<DataType>(k_count_[m]));
      }
    }
  }

  /**
   * Calculate kMeans cluster centres under the assumption that there may be some data points not
   * yet assigned
   */
  void PartialUpdate(ArrayType const &data)
  {
    k_means_.Fill(0);
    // get KSums
    SizeType cur_k;
    for (SizeType i = 0; i < n_points_; ++i)
    {
      if (!(k_assignment_[i] < 0))  // ignore if no assignment made yet
      {
        cur_k = static_cast<SizeType>(k_assignment_[i]);
        for (SizeType j = 0; j < n_dimensions_; ++j)
        {
          k_means_.Set(cur_k, j, k_means_.At(cur_k, j) + data.At(i, j));
        }
      }
    }

    // divide sums to get KMeans
    for (SizeType m = 0; m < n_clusters_; ++m)
    {
      for (SizeType i = 0; i < n_dimensions_; ++i)
      {
        k_means_.Set(m, i, k_means_.At(m, i) / static_cast<DataType>(k_count_[m]));
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
    for (SizeType i = 0; i < n_points_; ++i)
    {
      assert(cluster_assignment_map_.find(static_cast<SizeType>(k_assignment_.At(i))) !=
             cluster_assignment_map_.end());
      // overwrite every clust assignment with its equivalent previous label at input
      k_assignment_.Set(
          i, static_cast<DataType>(
                 cluster_assignment_map_.find(static_cast<SizeType>(k_assignment_.At(i)))->second));
    }
  }

  static constexpr SizeType INVALID       = numeric_max<SizeType>();
  SizeType                  n_points_     = INVALID;
  SizeType                  n_dimensions_ = INVALID;
  SizeType                  n_clusters_   = INVALID;

  SizeType no_change_count_ = INVALID;  // times there was no change in k_assignment in a row
  SizeType max_no_change_convergence_ = INVALID;  // max no change k_assignment before convergence
  SizeType loop_counter_              = INVALID;
  SizeType max_loops_                 = INVALID;
  DataType assigned_k_                = numeric_max<DataType>();  // current cluster to assign

  // used to find the smallest distance out of K comparisons
  DataType running_mean_ = numeric_max<DataType>();

  fetch::random::LaggedFibonacciGenerator<> lfg_;

  fetch::core::Vector<SizeType> data_idxs_;  // a vector of indices to the data used for shuffling
  fetch::core::Vector<SizeType> empty_clusters_;  // a vector tracking whenever a cluster goes empty

  ArrayType k_means_;       // current cluster centres
  ArrayType prev_k_means_;  // previous cluster centres (for checking convergence)
  ArrayType temp_k_;        // a container for ease of access to using Euclidean function

  ClusteringType k_assignment_;  // current data to cluster assignment
  ClusteringType
                 prev_k_assignment_;  // previous data to cluster assignment (for checkign convergence)
  ClusteringType reassigned_k_;       // reassigned data to cluster assignment

  fetch::core::Vector<SizeType>  k_count_;    // count of how many data points assigned per cluster
  fetch::core::Vector<ArrayType> k_euclids_;  // container for current euclid distances

  // map previously assigned clusters to current clusters
  std::unordered_map<SizeType, SizeType>
      cluster_assignment_map_{};  // <internal label, original label>

  bool reassign_{};

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
ClusteringType KMeans(ArrayType const &data, typename ArrayType::SizeType const &r_seed,
                      typename ArrayType::SizeType const &K,
                      typename ArrayType::SizeType        max_loops = 1000,
                      InitMode                            init_mode = InitMode::KMeansPP,
                      typename ArrayType::SizeType        max_no_change_convergence = 10)
{
  using SizeType = typename ArrayType::SizeType;
  using DataType = typename ArrayType::Type;

  SizeType n_points = data.shape()[0];

  // we can't have more clusters than data points
  assert(K <= n_points);  // you can't have more clusters than data points
  assert(K > 1);          // why would you run k means clustering with only one cluster?

  ClusteringType ret{n_points};

  if (n_points == K)  // very easy to cluster!
  {
    for (SizeType i = 0; i < n_points; ++i)
    {
      ret[i] = static_cast<DataType>(i);
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
ClusteringType KMeans(ArrayType const &data, typename ArrayType::SizeType const &r_seed,
                      ClusteringType const &prev_assignment, KInferenceMode const &k_inference_mode,
                      typename ArrayType::SizeType max_loops                 = 100,
                      typename ArrayType::SizeType max_no_change_convergence = 10)
{
  using SizeType = typename ArrayType::SizeType;

  SizeType       n_points = data.shape()[0];
  ClusteringType ret{n_points};
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
ClusteringType KMeans(ArrayType const &data, typename ArrayType::SizeType const &r_seed,
                      typename ArrayType::SizeType const &K, ClusteringType const &prev_assignment,
                      typename ArrayType::SizeType max_loops                 = 100,
                      typename ArrayType::SizeType max_no_change_convergence = 10)
{
  using SizeType = typename ArrayType::SizeType;
  using DataType = typename ArrayType::Type;

  SizeType       n_points = data.shape()[0];
  ClusteringType ret{n_points};

  assert(K <= n_points);  // you can't have more clusters than data points
  assert(K != 1);         // why would you run k means clustering with only one cluster?

  if (n_points == K)  // very easy to cluster!
  {
    for (SizeType i = 0; i < n_points; ++i)
    {
      ret[i] = static_cast<DataType>(i);
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
