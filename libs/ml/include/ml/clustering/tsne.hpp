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

#include "core/assert.hpp"
#include "core/random/lfg.hpp"
#include "math/distance/euclidean.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/meta/math_type_traits.hpp"
#include "math/metrics/kl_divergence.hpp"
#include "math/normalize_array.hpp"
#include "math/standard_functions/exp.hpp"
#include "math/standard_functions/log.hpp"
#include "math/tensor.hpp"
#include "meta/type_traits.hpp"
#include "ml/ops/flatten.hpp"

#include <cmath>

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
  using SizeType   = typename TensorType::SizeType;
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
    return DataType(1e-12);
  }

  TSNE(TensorType const &input_matrix, TensorType const &output_matrix, DataType const &perplexity)
  {
    Init(input_matrix, output_matrix, perplexity);
  }

  TSNE(TensorType const &input_matrix, SizeType const &output_dimensions,
       DataType const &perplexity, SizeType const &random_seed)
  {
    assert(input_matrix.shape().size() >= 2);
    TensorType output_matrix(
        {input_matrix.shape().at(input_matrix.shape().size() - 1), output_dimensions});
    rng_.Seed(random_seed);
    RandomInitWeights(output_matrix);
    Init(input_matrix, output_matrix, perplexity);
  }

  /**
   * i.e. Optimise cost function
   * @param learning_rate input Learning rate
   * @param max_iters input Number of optimisation iterations
   */
  void Optimise(DataType const &learning_rate, SizeType const &max_iters,
                DataType const &initial_momentum, DataType const &final_momentum,
                SizeType const &final_momentum_steps, SizeType const &p_later_correction_iteration)
  {
    // Initialise variables
    output_symmetric_affinities_.Fill(static_cast<DataType>(0));
    DataType min_gain{0.01f};
    DataType momentum = initial_momentum;
    assert(output_matrix_.shape().size() == 2);

    // i_y is output_matrix value from last iteration
    TensorType i_y(output_matrix_.shape());

    // Initialise gains with value 1.0
    TensorType gains(output_matrix_.shape());
    gains.Fill(static_cast<DataType>(1));

    // Start optimisation
    for (SizeType iter{0}; iter < max_iters; iter++)
    {
      // Compute output matrix pairwise affinities
      TensorType num;
      CalculateSymmetricAffinitiesQ(output_matrix_, output_symmetric_affinities_, num);

      // Compute gradient
      TensorType gradient = ComputeGradient(output_matrix_, input_symmetric_affinities_,
                                            output_symmetric_affinities_, num);

      // Perform the update
      if (iter >= final_momentum_steps)
      {
        momentum = final_momentum;
      }

      for (SizeType i{0}; i < output_matrix_.shape().at(0); i++)
      {
        for (SizeType j{0}; j < output_matrix_.shape().at(1); j++)
        {
          if ((gradient.At(i, j) > 0.0) != (i_y.At(i, j) > 0.0))
          {
            gains(i, j) = gains.At(i, j) + DataType(0.2);
          }

          if ((gradient.At(i, j) > 0.0) == (i_y.At(i, j) > 0.0))
          {
            gains(i, j) = gains.At(i, j) * DataType(0.8);
          }
        }
      }
      LimitMin(gains, min_gain);

      // i_y = momentum * i_y - learning_rate * (gains * gradient)
      i_y *= momentum;
      i_y -= fetch::math::Multiply(learning_rate, fetch::math::Multiply(gains, gradient));

      // output_matrix = output_matrix + i_y
      output_matrix_ = fetch::math::Add(output_matrix_, i_y);

      //  Y = Y - np.tile(np.mean(Y, 0), (n, 1))
      TensorType y_mean = fetch::math::Divide(fetch::math::ReduceSum(output_matrix_, 0),
                                              static_cast<DataType>(output_matrix_.shape().at(0)));

      output_matrix_ -= y_mean;

      // Compute current value of cost function
      std::cout << "Iteration " << iter << ", Loss: "
                << static_cast<double>(
                       KlDivergence(input_symmetric_affinities_, output_symmetric_affinities_))
                << std::endl;

      // Later P-values correction
      if (iter == p_later_correction_iteration)
      {
        input_symmetric_affinities_ = fetch::math::Divide(input_symmetric_affinities_, DataType(4));
      }
    }
  }

  const TensorType GetOutputMatrix() const
  {
    return output_matrix_.Transpose();
  }

private:
  /**
   * i.e. Sets initial values of TSNE and calculate P values
   */
  void Init(TensorType const &input_matrix, TensorType const &output_matrix,
            DataType const &perplexity)
  {
    // Flatten input
    if (input_matrix.shape().size() != 2)
    {
      fetch::ml::ops::Flatten<TensorType> flatten_op;
      TensorType                          flat_input(
          flatten_op.ComputeOutputShape({std::make_shared<TensorType>(input_matrix)}));
      flatten_op.Forward({std::make_shared<TensorType>(input_matrix)}, flat_input);
      input_matrix_ = flat_input.Transpose();
    }
    else
    {
      input_matrix_ = input_matrix.Transpose();
    }

    DataType perplexity_tolerance{1e-5f};
    SizeType max_tries{50};

    // Initialise high dimensional values
    SizeType input_data_size = input_matrix_.shape().at(0);

    // Find Pj|i values for given perplexity value within perplexity_tolerance
    input_pairwise_affinities_ = TensorType({input_data_size, input_data_size});
    CalculatePairwiseAffinitiesP(input_matrix_, input_pairwise_affinities_, perplexity,
                                 perplexity_tolerance, max_tries);

    // Calculate input_symmetric_affinities from input_pairwise_affinities
    // Pij=(Pj|i+Pi|j)/sum(Pij)
    input_symmetric_affinities_ =
        fetch::math::Add(input_pairwise_affinities_, input_pairwise_affinities_.Transpose());

    input_symmetric_affinities_ = fetch::math::Divide(
        input_symmetric_affinities_, fetch::math::Sum(input_symmetric_affinities_));
    // Early exaggeration
    input_symmetric_affinities_ = fetch::math::Multiply(input_symmetric_affinities_, DataType(4));

    // Limit minimum value to 1e-12
    LimitMin(input_symmetric_affinities_, tsne_tolerance<DataType>());

    // Initialise low dimensional values
    output_matrix_               = output_matrix;
    output_symmetric_affinities_ = TensorType(input_pairwise_affinities_.shape());
  }

  /**
   * i.e. Fill output_matrix with random values from normal distribution of mean 0.0 and standard
   * deviation 1.0
   */
  void RandomInitWeights(TensorType &output_matrix)
  {
    for (auto &val : output_matrix)
    {
      val = GetRandom(static_cast<DataType>(0), static_cast<DataType>(1));
    }
  }

  /**
   * Computes perplexity value and row pairwise affinities value p
   * @param d The precalculated values D = np.add(np.add(-2 * np.dot(X, X.T), sum_X).T, sum_X)
   * @param p The variable for pairwise affinities slice
   * @param entropy The variable for Shannon's entropy value
   * @param beta beta = 1/(2*sigma^2)
   * @param k i value excluded from sums from p(i,i)
   */
  void Hbeta(TensorType const &d, TensorType &p, DataType &entropy, DataType const &beta,
             SizeType const &k)
  {
    // p = -exp(d * beta)
    p = fetch::math::Exp(fetch::math::Multiply(DataType(-1), fetch::math::Multiply(d, beta)));
    p.Set(0, k, static_cast<DataType>(0));

    DataType sum_p = fetch::math::Sum(p);

    // entropy = log(sum_p) + beta * sum(d * p) / sum_p
    DataType sum_d_p = fetch::math::Sum(fetch::math::Multiply(p, d));

    entropy = fetch::math::Log(sum_p) + beta * sum_d_p / sum_p;

    // p = p / sum_p
    p = fetch::math::Divide(p, sum_p);
  }

  /**
   * i.e. Computes input matrix non-symmetric pairwise affinities Pj|i with perplexity value within
   * tolerance value
   * @param input_matrix input Tensor of input matrix values
   * @param pairwise_affinities output P values
   * @param target_perplexity input Target perplexity value
   * @param tolerance input Tolerance of perplexity value
   */
  void CalculatePairwiseAffinitiesP(TensorType const &input_matrix, TensorType &pairwise_affinities,
                                    DataType const &target_perplexity, DataType const &tolerance,
                                    SizeType const &max_tries)
  {
    SizeType input_data_size = input_matrix.shape().at(0);

    /*
     * Initialise some variables
     */
    // sum_x = sum(square(x), 1)
    TensorType sum_x = fetch::math::ReduceSum(fetch::math::Square(input_matrix), 1);

    // d= ((-2 * dot(X, X.T))+sum_x).T+sum_x
    TensorType d =
        fetch::math::Multiply(DataType(-2), fetch::math::DotTranspose(input_matrix, input_matrix));

    d = (d + sum_x).Transpose() + sum_x;

    // beta = 1/(2*sigma^2)
    // Prefill beta array with 1.0
    TensorType beta(input_data_size);
    beta.Fill(static_cast<DataType>(1));

    // Calculate entropy value from perplexity
    // DataType target_entropy = std::log(target_perplexity);
    DataType target_entropy = fetch::math::Log(target_perplexity);

    /*
     * Loop over all datapoints
     */
    for (SizeType i{0}; i < input_data_size; i++)
    {
      // Compute the Gaussian kernel and entropy for the current precision
      DataType inf     = math::numeric_max<DataType>();
      DataType neg_inf = math::numeric_lowest<DataType>();

      DataType beta_min = neg_inf;
      DataType beta_max = inf;

      // Slice of pairwise affinities
      TensorType this_P(input_data_size);

      DataType current_entropy;
      d.Set(i, i, static_cast<DataType>(0));
      Hbeta(d.Slice(i).Copy(), this_P, current_entropy, beta.At(i), i);

      // Evaluate whether the perplexity is within tolerance
      DataType entropy_diff = current_entropy - target_entropy;
      SizeType tries        = 0;

      while (fetch::math::Abs(entropy_diff) > tolerance && tries < max_tries)
      {

        // If not, increase or decrease precision
        if (entropy_diff > 0)
        {
          beta_min = beta.At(i);
          if (beta_max == inf || beta_max == neg_inf)
          {
            beta.Set(i, beta.At(i) * DataType(2));
          }
          else
          {
            beta.Set(i, (beta.At(i) + beta_max) / DataType(2));
          }
        }
        else
        {
          beta_max = beta.At(i);
          if (beta_min == inf || beta_min == neg_inf)
          {
            beta.Set(i, beta.At(i) / DataType(2));
          }
          else
          {
            beta.Set(i, (beta.At(i) + beta_min) / DataType(2));
          }
        }

        // Recompute the values
        Hbeta(d.Slice(i).Copy(), this_P, current_entropy, beta.At(i), i);
        entropy_diff = current_entropy - target_entropy;
        tries++;
      }

      //  Set the final row of pairwise affinities
      for (SizeType j{0}; j < pairwise_affinities.shape().at(1); j++)
      {
        if (i == j)
        {
          pairwise_affinities.Set(i, j, static_cast<DataType>(0));
          continue;
        }
        pairwise_affinities.Set(i, j, this_P.At(0, j));
      }
    }
  }

  /**
   * i.e. Computes output matrix symmetric pairwise affinities Qij and precalculated values num
   * @param output_matrix input Tensor of output matrix values
   * @param output_symmetric_affinities output Q values
   * @param num output Precalculated values of Student-t based distribution
   */
  void CalculateSymmetricAffinitiesQ(TensorType const &output_matrix,
                                     TensorType &output_symmetric_affinities, TensorType &num)
  {
    /*
     * Compute Q pairwise affinities
     */
    TensorType output_matrix_T = output_matrix.Transpose();

    // sum_y = sum(square(y), 1)
    TensorType sum_y = fetch::math::ReduceSum(fetch::math::Square(output_matrix), 1);

    // num = -2. * dot(Y, Y.T)
    num = fetch::math::Multiply(DataType(-2),
                                fetch::math::DotTranspose(output_matrix, output_matrix));

    // num = 1 / (1 + (num+sum_y).T+sum_y)
    TensorType val((num + sum_y).Transpose());
    num = fetch::math::Divide(static_cast<DataType>(1),
                              fetch::math::Add(static_cast<DataType>(1), (val + sum_y)));

    // num[range(n), range(n)] = 0.
    for (SizeType i{0}; i < num.shape().at(0); i++)
    {
      num.Set(i, i, static_cast<DataType>(0));
    }

    // Q = num / sum(num)
    output_symmetric_affinities = fetch::math::NormalizeArray(num);

    // Crop minimal value to 1e-12
    LimitMin(output_symmetric_affinities, tsne_tolerance<DataType>());
  }

  /**
   * i.e. Returns random value from normal distribution of standard_deviation and mean
   * @param mean input DataType mean value of normal distribution
   * @param standard_deviation input DataType standart deviation value of normal distribution
   * @return random DataType value
   */
  DataType GetRandom(DataType /*mean*/, DataType /*standard_deviation*/)
  {
    // TODO(issue 752): use normal distribution random instead
    return DataType(rng_.AsDouble());
  }

  /**
   * i.e. Calculates the gradient of the Kullback-Leibler divergence between P and the Student-t
   * based joint probability distribution Q
   * @param output_matrix output low dimensional values tensor
   * @param input_symmetric_affinities Tensor of symmetrized input matrix pairwise affinities
   * @param output_symmetric_affinities Tensor of symmetrized output matrix pairwise affinities
   * @param num Tensor of precalculated values num[i,j]=1+||yi-yj||^2
   * @param ret return value output_matrix shaped tensor of gradient values.
   */
  TensorType ComputeGradient(TensorType const &output_matrix,
                             TensorType const &input_symmetric_affinities,
                             TensorType const &output_symmetric_affinities, TensorType const &num)
  {
    assert(input_matrix_.shape().at(0) == output_matrix.shape().at(0));

    TensorType ret(output_matrix.shape());

    for (SizeType i{0}; i < output_matrix.shape().at(0); i++)
    {
      TensorType output(output_matrix.shape().at(1));
      for (SizeType j{0}; j < output_matrix.shape().at(0); j++)
      {
        if (i == j)
        {
          continue;
        }

        DataType p_i_j;
        p_i_j = input_symmetric_affinities.At(i, j);

        DataType q_i_j = output_symmetric_affinities.At(i, j);

        // (Pij-Qij)
        DataType val = p_i_j - q_i_j;

        // /(1+||yi-yj||^2)
        fetch::math::Multiply(num.At(i, j), val, val);

        // val*(yi-yj), where val=(Pij-Qij)/(1+||yi-yj||^2)

        TensorType diff = (output_matrix.Slice(j).Copy()) - (output_matrix.Slice(i).Copy());

        SizeVector shape = diff.shape();
        shape.erase(shape.begin());
        diff.Reshape(shape);

        output += Multiply(val, diff);
      }

      for (SizeType k = 0; k < output_matrix.shape().at(1); k++)
      {
        ret(i, k) = fetch::math::Multiply(static_cast<DataType>(-1), output.At(k));
      }
    }

    return ret;
  }

  /**
   * i.e. if any value of input matrix is lower than min, sets it to min
   * @param matrix input tensor
   * @param min limit value to be applied
   * @return
   */
  void LimitMin(TensorType &matrix, DataType const &min)
  {
    for (auto &val : matrix)
    {
      val = (val < min) ? min : val;
    }
  }

  TensorType input_matrix_, output_matrix_;
  TensorType input_pairwise_affinities_, input_symmetric_affinities_;
  TensorType output_symmetric_affinities_;
  RNG        rng_;
};

}  // namespace ml
}  // namespace fetch
