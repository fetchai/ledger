#ifndef OPTIMISATION_SIMULATED_ANNEALING_SPARSE_ANNEALER_HPP
#define OPTIMISATION_SIMULATED_ANNEALING_SPARSE_ANNEALER_HPP
#include <math/exp.hpp>
#include <memory/rectangular_array.hpp>
#include <random/lcg.hpp>

#include <vector>

namespace fetch {
namespace optimisers {

class SparseAnnealer {
 public:
  typedef math::Exp<0> exp_type;
  typedef double cost_type;
  typedef std::vector<int8_t> state_type;
  typedef random::LinearCongruentialGenerator random_generator_type;
  typedef random::LinearCongruentialGenerator::random_type random_type;

  SparseAnnealer() : beta0_(0.1), beta1_(3), sweeps_(1000), size_(0) {}
  SparseAnnealer(std::size_t const &n)
      : couplings_(n, n), beta0_(0.1), beta1_(3), sweeps_(1000), size_(0) {}

  void Resize(std::size_t const &n, std::size_t const &max_connectivity = std::size_t(-1) ) {
    std::cout << "Connectivity: "<< max_connectivity << std::endl;
    
    couplings_.Resize(n, n);
    for (std::size_t i = 0; i < couplings_.size(); ++i) couplings_[i] = 0;
    size_ = n;
  }

  void Anneal(state_type &state) {
    Initialize(state);
    SetBeta(beta0_);

    double db = (beta1_ - beta0_) / double(sweeps_ - 1);
    for (std::size_t k = 0; k < sweeps_; ++k) {
      for (std::size_t i = 0; i < size_; ++i) {
        if (rng_.AsDouble() <= fexp_(local_energies_[i])) {
          cost_type diff = -2 * state[i];

          for (std::size_t j = 0; j < i; ++j)
            local_energies_[j] += diff * state[j] * couplings_(j, i);

          for (std::size_t j = i + 1; j < size_; ++j)
            local_energies_[j] += diff * state[j] * couplings_(i, j);

          local_energies_[i] = -local_energies_[i];
          state[i] = -state[i];
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

  cost_type CostOf(state_type c, bool binary = true) {
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

    return ret;
  }

  std::size_t const &size() const { return size_; }

  void SetBeta(double beta) {
    beta_ = beta;
    fexp_.SetCoefficient(2 * beta_);
  }

  double beta() const { return beta_; }

  void SetSweeps(std::size_t sweeps) { sweeps_ = sweeps; }
  void SetBetaStart(cost_type const &b0) { beta0_ = b0; }
  void SetBetaEnd(cost_type const &b1) { beta1_ = b1; }

  static void SpinToBinary(state_type &state) {
    for (auto &s : state) s = ((1 - s) >> 1);
  }

  static void BinaryToSpin(state_type &state) {
    for (auto &s : state) s = 1 - 2 * s;
  }

 private:
  cost_type energy(state_type &state) const {
    cost_type en = 0;
    for (std::size_t i = 0; i < size_; ++i)
      en += local_energies_[i] + couplings_(i, i) * state[i];
    return 0.5 * en;
  }

  void Initialize(state_type &state) {
    state.resize(size_);
    for (auto &s : state) s = 1 - 2 * ((rng_() >> 27) & 1);

    local_energies_.resize(size_);
    ComputeLocalEnergies(state);
  }

  void ComputeLocalEnergies(state_type &state) {
    for (std::size_t i = 0; i < size_; ++i) {
      cost_type de = couplings_(i, i);

      for (std::size_t j = 0; j < i; ++j) de += state[j] * couplings_(j, i);

      for (std::size_t j = i + 1; j < size_; ++j)
        de += state[j] * couplings_(i, j);

      local_energies_[i] = de * state[i];
    }
  }

  exp_type fexp_;


  struct Spin 
  {
    std::vector< cost_type > couplings_;
    std::vector< std::size_t > indices_;
    cost_type local_field;    
  };
  

  
  std::vector< Spin > spins_;
  
  double beta_, beta0_, beta1_;
  std::size_t sweeps_;
  std::size_t size_ = 0;

  random_generator_type rng_;
  std::vector<cost_type> local_energies_;
};
};
};
#endif
