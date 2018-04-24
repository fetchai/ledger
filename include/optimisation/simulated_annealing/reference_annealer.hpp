#ifndef OPTIMISATION_SIMULATED_ANNEALING_HPP
#define OPTIMISATION_SIMULATED_ANNEALING_HPP
#include "math/exp.hpp"
#include "memory/rectangular_array.hpp"
#include "random/lcg.hpp"
#include "optimisation/abstract_spinglass_solver.hpp"

#include <vector>

namespace fetch {
namespace optimisers {

class ReferenceAnnealer : public AbstractSpinGlassSolver {
 public:
  typedef math::Exp< 0 > exp_type;

  typedef int8_t state_primitive_type;
  typedef std::vector< state_primitive_type > state_type;
  typedef random::LinearCongruentialGenerator random_generator_type;
  typedef random::LinearCongruentialGenerator::random_type random_type;

  ReferenceAnnealer() : beta0_(0.1), beta1_(3), sweeps_(1000), size_(0) {}
  ReferenceAnnealer(std::size_t const &n)
      : couplings_(n, n), beta0_(0.1), beta1_(3), sweeps_(1000), size_(0) {}

  void Resize(std::size_t const &n, std::size_t max_connectivity = std::size_t(-1)) override {
    couplings_.Resize(n, n);
    for (std::size_t i = 0; i < couplings_.size(); ++i) couplings_[i] = 0;
    size_ = n;
  }

  void Anneal(state_type &state) {
    Initialize(state);
    SetBeta(beta0_);

    double db = (beta1_ - beta0_) / double(sweeps_ - 1);
    for (std::size_t k = 0; k < sweeps_; ++k) {
      attempts_ += double(size_);
      for (std::size_t i = 0; i < size_; ++i) {
        if (rng_.AsDouble() <= fexp_(local_energies_[i])) {
          cost_type diff = -2 * state[i];

          for (std::size_t j = 0; j < i; ++j)
            local_energies_[j] += diff * state[j] * couplings_(j, i);

          for (std::size_t j = i + 1; j < size_; ++j)
            local_energies_[j] += diff * state[j] * couplings_(i, j);

          local_energies_[i] = -local_energies_[i];
          state[i] = state_primitive_type (-state[i]);
          ++accepted_;
        }
      }

      SetBeta(beta() + db);
    }
  }

  cost_type const &operator()(std::size_t const &i,
                              std::size_t const &j) const {
    return couplings_(std::min(i, j), std::max(i, j));
  }

  cost_type &operator()(std::size_t const &i, std::size_t const &j) {
    return couplings_(std::min(i, j), std::max(i, j));
  }

  cost_type const &At(std::size_t const &i, std::size_t const &j) const {
    return couplings_(std::min(i, j), std::max(i, j));
  }

  cost_type &At(std::size_t const &i, std::size_t const &j) {
    return couplings_(std::min(i, j), std::max(i, j));
  }

  cost_type const &Set(std::size_t const &i, std::size_t const &j,
                       cost_type const &v) {
    return couplings_.Set(i, j, v);
  }

  cost_type FindMinimum() {
    state_type ret;
    return FindMinimum(ret);
  }

  cost_type FindMinimum(state_type &state, bool binary = true) {
    Anneal(state);
    cost_type ret = energy(state);
    if (binary) SpinToBinary(state);
    return ret;
  }

  cost_type CostOf(state_type &c, bool binary = true) {
    cost_type ret = 0;

    if (binary) BinaryToSpin(c);

    for (std::size_t i = 0; i < size_; ++i) {
      int s1 = c[i];
      ret += s1 * couplings_(i, i);
      for (std::size_t j = i + 1; j < size_; ++j) {
        int s2 = c[j];
        ret += s1 * s2 * couplings_(i, j);
      }
    }

    if (binary) SpinToBinary(c);
    
    return ret;
  }

  std::size_t const &size() const { return size_; }

  void SetBeta(double beta) {
    beta_ = beta;
    fexp_.SetCoefficient(2 * beta_);
  }

  double beta() const { return beta_; }
  std::size_t sweeps() const { return sweeps_; }
  
  void SetSweeps(std::size_t sweeps) { sweeps_ = sweeps; }
  void SetBetaStart(cost_type const &b0) { beta0_ = b0; }
  void SetBetaEnd(cost_type const &b1) { beta1_ = b1; }

  static void SpinToBinary(state_type &state) {
    for (auto &s : state) s = state_primitive_type((1 - s) >> 1);
  }

  static void BinaryToSpin(state_type &state) {
    for (auto &s : state) s = state_primitive_type(1 - 2 * s);
  }
  
  void Insert(std::size_t const &i, std::size_t const &j, cost_type const &c) override {
    couplings_.Set(std::min(i, j), std::max(i, j),c);
  }

  void Update(std::size_t const &i, std::size_t const &j, cost_type const &c) override {
    std::size_t A =std::min(i, j);
    std::size_t B = std::max(i, j);
    couplings_(A,B) += c;
  }
    
  void PrintGraph() {
    for(std::size_t i=0; i < size_; ++i) {
      for(std::size_t j=0; j < size_; ++j) {
        if(couplings_(i,j) != 0 ) {
          std::cout << i << " " << j << " " << couplings_(i,j) << std::endl;
        }
      }
    }
  }

  double attempts() { return attempts_; }
  double accepted() { return accepted_; }
  
private:
  double attempts_ = 0;
  double accepted_ = 0;
  
  cost_type energy(state_type &state) const {
    cost_type en = 0;
    for (std::size_t i = 0; i < size_; ++i)
      en += local_energies_[i] + couplings_(i, i) * state[i];
    return 0.5 * en;
  }

  void Initialize(state_type &state) {
    attempts_ = 0;
    accepted_ = 0;
    state.resize(size_);
    for (auto &s : state) s = state_primitive_type(1 - 2 * ((rng_() >> 27) & 1) );

    local_energies_.resize(size_);
    ComputeLocalEnergies(state);
  }

  void ComputeLocalEnergies(state_type &state) {
    for (std::size_t i = 0; i < size_; ++i) {
      cost_type de = couplings_(i, i);

      for (std::size_t j = 0; j < i; ++j)
        de += state[j] * couplings_(j, i);

      for (std::size_t j = i + 1; j < size_; ++j)
        de += state[j] * couplings_(i, j);

      local_energies_[i] = de * state[i];
    }
  }

  exp_type fexp_;

  memory::RectangularArray<cost_type> couplings_;
  double beta_, beta0_, beta1_;
  std::size_t sweeps_;
  std::size_t size_ = 0;

  random_generator_type rng_;
  std::vector<cost_type> local_energies_;
};
}
}
#endif
