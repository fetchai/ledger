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

#include "core/random/lfg.hpp"
#include "math/base_types.hpp"

namespace fetch {
namespace ml {

/**
 *  Implementation of T-SNE clustering algorithm based on paper:
 *  http://www.jmlr.org/papers/volume9/vandermaaten08a/vandermaaten08a.pdf
 *  i.e. Algorithm for high dimensional data reduction for visualisation
 */
template <class T>
class TSNE
{
public:
  using TensorType = T;
  using DataType   = typename TensorType::Type;
  using SizeType   = fetch::math::SizeType;
  using SizeVector = typename TensorType::SizeVector;
  using RNG        = fetch::random::LaggedFibonacciGenerator<>;

  static constexpr char const *DESCRIPTOR = "TSNE";

  template <typename DataType>
  static constexpr math::meta::IfIsFixedPoint<DataType, DataType> tsne_tolerance()
  {
    return DataType::CONST_SMALLEST_FRACTION;
  }

  template <typename DataType>
  static constexpr math::meta::IfIsNonFixedPointArithmetic<DataType, DataType> tsne_tolerance()
  {
    return fetch::math::Type<DataType>("0.00000000001");
  }

  TSNE(TensorType const &input_matrix, TensorType const &output_matrix, DataType const &perplexity);

  TSNE(TensorType const &input_matrix, SizeType const &output_dimensions,
       DataType const &perplexity, SizeType const &random_seed);

  void Optimise(DataType const &learning_rate, SizeType const &max_iters,
                DataType const &initial_momentum, DataType const &final_momentum,
                SizeType const &final_momentum_steps, SizeType const &p_later_correction_iteration);

  const TensorType GetOutputMatrix() const;

private:
  void Init(TensorType const &input_matrix, TensorType const &output_matrix,
            DataType const &perplexity);

  void RandomInitWeights(TensorType &output_matrix);

  void Hbeta(TensorType const &d, TensorType &p, DataType &entropy, DataType const &beta,
             SizeType const &k);

  void CalculatePairwiseAffinitiesP(TensorType const &input_matrix, TensorType &pairwise_affinities,
                                    DataType const &target_perplexity, DataType const &tolerance,
                                    SizeType const &max_tries);

  void CalculateSymmetricAffinitiesQ(TensorType const &output_matrix,
                                     TensorType &output_symmetric_affinities, TensorType &num);

  DataType GetRandom(DataType /*mean*/, DataType /*standard_deviation*/);

  TensorType ComputeGradient(TensorType const &output_matrix,
                             TensorType const &input_symmetric_affinities,
                             TensorType const &output_symmetric_affinities, TensorType const &num);

  void LimitMin(TensorType &matrix, DataType const &min);

  TensorType input_matrix_, output_matrix_;
  TensorType input_pairwise_affinities_, input_symmetric_affinities_;
  TensorType output_symmetric_affinities_;
  RNG        rng_;
};

}  // namespace ml
}  // namespace fetch
