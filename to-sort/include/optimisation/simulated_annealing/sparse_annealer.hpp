#pragma once
#include "math/exp.hpp"
#include "memory/rectangular_array.hpp"
#include "optimisation/abstract_spinglass_solver.hpp"
#include "random/lcg.hpp"

#include <vector>

namespace fetch {
namespace optimisers {

class SparseAnnealer : public AbstractSpinGlassSolver {
 public:
  typedef math::Exp<0> exp_type;

  typedef int8_t spin_type;
  typedef std::vector<spin_type> state_type;
  typedef random::LinearCongruentialGenerator random_generator_type;
  typedef random::LinearCongruentialGenerator::random_type random_type;

  SparseAnnealer() : beta0_(0.1), beta1_(3), sweeps_(1000), size_(0) {}
  SparseAnnealer(std::size_t const &n)
      : beta0_(0.1), beta1_(3), sweeps_(1000), size_(0) {}

  void Resize(std::size_t const &n,
              std::size_t max_connectivity = std::size_t(-1)) override {
    if (max_connectivity == std::size_t(-1)) {
      max_connectivity = n;
    }

    sites_.resize(n);
    for (auto &s : sites_) {
      s.couplings.reserve(max_connectivity);
      s.indices.reserve(max_connectivity);
    }
    size_ = n;
  }

  // mads
  void Anneal(state_type &state) {
    Initialize();
    SetBeta(beta0_);

    double db = (beta1_ - beta0_) / double(sweeps_ - 1);
    for (std::size_t k = 0; k < sweeps_; ++k) {
      for (std::size_t i = 0; i < size_; ++i) {
        auto &site = sites_[i];
        if (rng_.AsDouble() <= fexp_(site.local_energy)) {
          cost_type de = -2 * site.spin_value;

          for (std::size_t j = 0; j < site.indices.size(); ++j) {
            Site &site2 = sites_[site.indices[j]];
            site2.local_energy += de * site.couplings[j] * site2.spin_value;
          }

          site.local_energy = -site.local_energy;
          site.spin_value = spin_type(-site.spin_value);
        }
      }

      SetBeta(beta() + db);
    }

    state.resize(sites_.size());
    for (std::size_t i = 0; i < state.size(); ++i) {
      auto &site = sites_[i];
      state[i] = site.spin_value;
    }
  }

  cost_type FindMinimum() {
    state_type ret;
    return FindMinimum(ret);
  }

  cost_type FindMinimum(state_type &state, bool binary = true) {
    Anneal(state);
    cost_type ret = energy();
    if (binary) SpinToBinary(state);
    return ret;
  }
  /*
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
  */

  void PrintGraph() {}

  std::size_t const &size() const { return size_; }

  void SetBeta(double beta) {
    beta_ = beta;
    fexp_.SetCoefficient(2 * beta_);
  }

  void SetSweeps(std::size_t sweeps) { sweeps_ = sweeps; }
  void SetBetaStart(cost_type const &b0) { beta0_ = b0; }
  void SetBetaEnd(cost_type const &b1) { beta1_ = b1; }

  static void SpinToBinary(state_type &state) {
    for (auto &s : state) s = spin_type((1 - s) >> 1);
  }

  static void BinaryToSpin(state_type &state) {
    for (auto &s : state) s = spin_type(1 - 2 * s);
  }

  cost_type CostOf(state_type &c, bool binary = true) {
    cost_type ret = 0;

    if (binary) BinaryToSpin(c);

    for (std::size_t i = 0; i < size_; ++i) {
      Site &site = sites_[i];
      auto s1 = c[i];

      ret += 2 * s1 * site.local_field;
      for (std::size_t j = 0; j < site.indices.size(); ++j) {
        auto k = site.indices[j];
        auto s2 = c[k];

        ret += s1 * s2 * site.couplings[j];
      }
    }

    if (binary) SpinToBinary(c);

    return 0.5 * ret;
  }

  void Insert(std::size_t const &i, std::size_t const &j,
              cost_type const &c) override {
    if (i == j) {
      Site &s1 = sites_[i];
      s1.local_field = c;
    } else {
      Site &s1 = sites_[i];
      s1.indices.push_back(j);
      s1.couplings.push_back(c);
      Site &s2 = sites_[j];
      s2.indices.push_back(i);
      s2.couplings.push_back(c);
    }
  }

  void Update(std::size_t const &i, std::size_t const &j,
              cost_type const &c) override {
    TODO_FAIL("Node implemented yet");
  }

  double beta() const { return beta_; }
  std::size_t sweeps() const { return sweeps_; }

 private:
  cost_type energy() const {
    cost_type en = 0;
    for (auto &s : sites_) {
      en += s.local_energy + s.local_field * s.spin_value;
    }
    return 0.5 * en;
  }

  void Initialize() {
    for (auto &s : sites_)
      s.spin_value = spin_type(1 - 2 * ((rng_() >> 27) & 1));

    ComputeLocalEnergies();
  }

  void ComputeLocalEnergies() {
    for (std::size_t i = 0; i < size_; ++i) {
      Site &site = sites_[i];
      cost_type de = site.local_field;

      for (std::size_t j = 0; j < site.couplings.size(); ++j) {
        spin_type spin = sites_[site.indices[j]].spin_value;
        de += spin * site.couplings[j];
      }

      site.local_energy = de * site.spin_value;
    }
  }

  exp_type fexp_;

  struct Site {
    std::vector<cost_type> couplings;
    std::vector<std::size_t> indices;
    cost_type local_field = 0;
    cost_type local_energy = 0;
    spin_type spin_value = 1;
  };

  std::vector<Site> sites_;

  double beta_, beta0_, beta1_;
  std::size_t sweeps_ = 0;
  std::size_t size_ = 0;

  random_generator_type rng_;
  std::vector<cost_type> local_energies_;
};
}
}
