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

#include "core/bitvector.hpp"
#include "core/random/lcg.hpp"
#include "core/random/lfg.hpp"
#include "logging/logging.hpp"
#include "math/approx_exp.hpp"

namespace fetch {
namespace optimisers {

class BinaryAnnealer
{
public:
  static constexpr char const *LOGGING_NAME = "BinaryAnnealer";

  using SpinType  = int16_t;
  using StateType = std::vector<SpinType>;

  using exp_type      = math::ApproxExpImplementation<0>;
  using BitDataType   = uint64_t;
  using BitVectorType = BitVector;
  using CostType      = double;

  void Anneal()
  {
    Initialise();
    SetBeta(beta0_);

    double        db = (beta1_ - beta0_) / double(sweeps_ - 1);
    BitVectorType B;
    B.Resize(size_);

    for (std::size_t k = 0; k < sweeps_; ++k)
    {
      attempts_ += double(size_);

      std::size_t block = 0;
      std::size_t bit   = 0;

      double   r           = sim_rng_.AsDouble();
      double   TlogR       = std::log(r) / beta_ / 2. / normalisation_constant_;
      CostType approxTlogR = CostType(TlogR);

      for (std::size_t i = 0; i < size_; ++i)
      {
        //        assert( i ==( block * 8*sizeof(BitDataType) + bit ));
        assert(i < sites_.size());

        Site const &s         = sites_[i];
        uint64_t    state_bit = state_.bit(block, bit);
        uint64_t    update    = 0;

        int p = 0;
        switch (B.blocks())
        {
        case 0:
          break;
        case 1:
        case 2:
          B(0) = s.couplings(0) & state_(0);
          B(1) = s.couplings(1) & state_(1);
          p    = __builtin_popcountl(B(0)) + __builtin_popcountl(B(1));

          break;

        case 3:
        case 4:
          B(0) = s.couplings(0) & state_(0);
          B(1) = s.couplings(1) & state_(1);
          B(2) = s.couplings(2) & state_(2);
          B(3) = s.couplings(3) & state_(3);
          p    = __builtin_popcountl(B(0)) + __builtin_popcountl(B(1)) + __builtin_popcountl(B(2)) +
              __builtin_popcountl(B(3));
          break;

        default:
          B.InlineAndAssign(s.couplings, state_);
          p = static_cast<int>(B.PopCount());
          break;
        }

        CostType C;
        C      = -CostType(p) - s.local_field;
        update = (approxTlogR <= C);
        //        update =
        switch (state_bit)
        {
        case 0:

          break;
        case 1:
          C      = CostType(p) + s.local_field;
          update = (approxTlogR <= C);
          break;
        }

        if (update)
        {
          state_.conditional_flip(block, bit, update);
        }
        ++bit;
        block += (bit >= 8 * sizeof(BitDataType));
        bit &= (8 * sizeof(BitDataType) - 1);
      }

      SetBeta(beta() + db);
    }
  }

  void Normalise()
  {
    if (coupling_magnitude_ == 1.0)
    {
      return;
    }
    if (coupling_magnitude_ == 0.0)
    {
      return;
    }

    for (std::size_t i = 0; i < size_; ++i)
    {
      assert(i < sites_.size());

      sites_[i].local_field /= coupling_magnitude_;
    }

    normalisation_constant_ *= coupling_magnitude_;
    coupling_magnitude_ = 1.0;
  }

  CostType FindMinimum()
  {
    StateType ret;
    return FindMinimum(ret);
  }

  CostType FindMinimum(StateType &state, bool /*binary*/ = true)
  {
    Anneal();
    CostType ret = Energy();

    // TODO(issue 30): Optimise

    state.clear();

    for (std::size_t i = 0; i < size_; ++i)
    {
      state.push_back(SpinType(state_.bit(i)));
    }

    return ret;
  }

  CostType Energy() const
  {
    CostType    ret   = 0;
    std::size_t block = 0;
    std::size_t bit   = 0;

    for (std::size_t i = 0; i < size_; ++i)
    {
      assert(i == (block * 8 * sizeof(BitDataType) + bit));
      Site const &s = sites_[i];

      BitVectorType B    = s.couplings & state_;
      CostType      sign = CostType(state_.bit(block, bit));

      ret += sign * (2 * s.local_field + coupling_magnitude_ * static_cast<int>(B.PopCount()));
      ++bit;
      block += (bit >= 8 * sizeof(BitDataType));
      bit &= (8 * sizeof(BitDataType) - 1);
    }
    return CostType(ret * 0.5 * normalisation_constant_);
  }

  void Resize(std::size_t n, std::size_t /*m*/ = std::size_t(-1))
  {
    sites_.resize(n);
    for (std::size_t i = 0; i < n; ++i)
    {
      Site &s = sites_[i];
      s.couplings.Resize(n);
      s.couplings.SetAllZero();

      s.local_field = 0;
    }

    state_.Resize(n);
    state_.SetAllZero();
    size_               = n;
    coupling_magnitude_ = 0;
  }

  void Insert(std::size_t i, std::size_t j, CostType const &val)
  {
    if (i == j)
    {
      assert(j < sites_.size());

      sites_[j].local_field = val;
    }
    else
    {
      assert((coupling_magnitude_ == 0) || (coupling_magnitude_ == val));
      assert(i < sites_.size());
      assert(j < sites_.size());

      sites_[i].couplings.set(j, 1);
      sites_[j].couplings.set(i, 1);
      coupling_magnitude_ = val;
    }
  }

  std::size_t size() const
  {
    return size_;
  }

  void SetBeta(double beta)
  {
    beta_ = beta;
    fexp_.SetCoefficient(2 * beta_);
  }

  double beta() const
  {
    return beta_;
  }
  std::size_t sweeps() const
  {
    return sweeps_;
  }

  void SetSweeps(std::size_t sweeps)
  {
    sweeps_ = sweeps;
  }
  void SetBetaStart(double const &b0)
  {
    beta0_ = b0;
  }
  void SetBetaEnd(double const &b1)
  {
    beta1_ = b1;
  }

  void Initialise()
  {
    attempts_ = 0;
    accepted_ = 0;
    state_.Resize(size_);
    for (std::size_t i = 0; i < state_.blocks(); ++i)
    {
      state_(i) = (init_rng_() >> 32) ^ init_rng_();
    }
  }

  BitVectorType state()
  {
    return state_;
  }

  void Reset()
  {
    attempts_           = 0;
    accepted_           = 0;
    coupling_magnitude_ = 0, normalisation_constant_ = 1.0;

    state_.SetAllZero();
    sites_.clear();

    size_ = 0;
  }

private:
  double   attempts_           = 0;
  double   accepted_           = 0;
  CostType coupling_magnitude_ = 0, normalisation_constant_ = 1.0;

  struct Site
  {
    BitVectorType couplings;
    CostType      local_field;
  };

  BitVectorType     state_;
  std::vector<Site> sites_;
  double            beta_, beta0_ = 0.1, beta1_ = 3;

  std::size_t                         sweeps_ = 10;
  std::size_t                         size_   = 0;
  exp_type                            fexp_;
  random::LinearCongruentialGenerator sim_rng_;
  random::LaggedFibonacciGenerator<>  init_rng_;
};

}  // namespace optimisers
}  // namespace fetch
