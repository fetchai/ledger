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

#include <cmath>

namespace fetch {
namespace math {
namespace statistics {
namespace normal {

namespace helper {
template <typename T>
constexpr T pi()
{
  return std::atan(static_cast<T>(1)) * 4;
}
}  // namespace helper

/**
 * Probability density of normal distribution with mu and std.
 */
template <typename T>
inline T pdf(T const &m, T const &s, T const &x)
{
  return std::exp(-(x - m) * (x - m) / (2. * s * s)) / (s * sqrt(2. * helper::pi<T>()));
}

/**
 * Cumulative distribution function of normal distribution.
 */
template <typename T>
inline T cdf(T const &m, T const &s, T const &x)
{
  return 0.5 * std::erfc(-(x - m) / (s * sqrt(2.)));
}

/**
 * This function implements the Pade approximation for the inverse of the erf function.
 * It is a good approximation in [-0.9,0.9].
 */
template <typename T>
inline T erf_inv(T const &x)
{
  return (std::sqrt(helper::pi<T>()) / 2.) * x *
         (1 - 4397 * helper::pi<T>() * std::pow(x, 2) / 17352. +
          111547 * std::pow(helper::pi<T>(), 2) * std::pow(x, 4) / 14575680.) /
         (1 - 5843 * helper::pi<T>() * std::pow(x, 2) / 17352. +
          20533 * std::pow(helper::pi<T>(), 2) * std::pow(x, 4) / 971712.);
}

template <typename T>
inline T erfc_inv(T const &z)
{
  return erf_inv(1 - z);
}

template <typename T>
inline T quantile(T const &m, T const &s, T const &p)
{
  return m - s * std::sqrt(2) * erfc_inv(2 * p);
}

}  // namespace normal

/**
 * Gaussian class implements various operations with Gaussian distributions.
 */
template <typename T>
class Gaussian
{
public:
  Gaussian(){};
  Gaussian(T pi, T tau)
    : pi_(pi)
    , tau_(tau)
  {}

  static Gaussian ClassicForm(T mu, T sigma)
  {
    Gaussian g;
    g.set_mu_sigma(mu, sigma);
    return g;
  }

  Gaussian operator*(const Gaussian &g) const
  {
    // Multiply two Gaussians.
    T new_pi  = this->pi_ + g.pi_;
    T new_tau = this->tau_ + g.tau_;
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator*=(const Gaussian &g)
  {
    // Multiply myself and another Gaussian.
    this->pi_ += g.pi_;
    this->tau_ += g.tau_;
    return *this;
  }

  Gaussian operator*(T s) const
  {
    // Multiply by scalar. This action increases only the variance.
    T k       = 1 + this->pi_ * s * s;
    T new_pi  = this->pi_ / k;
    T new_tau = this->tau_ / k;
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator*=(T s)
  {
    // Multiply myself by scalar. This action increases only the variance.
    T k = 1 + this->pi_ * s * s;
    this->pi_ /= k;
    this->tau_ /= k;
    return *this;
  }

  Gaussian operator/(const Gaussian &g) const
  {
    // Divide two Gaussians.
    T new_pi  = this->pi_ - g.pi_;
    T new_tau = this->tau_ - g.tau_;
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator/=(const Gaussian &g)
  {
    // Divide by Gaussian.
    this->pi_ -= g.pi_;
    this->tau_ -= g.tau_;
    return *this;
  }

  Gaussian operator+(const Gaussian &g) const
  {
    // Add two Gaussians.
    T new_pi  = 1. / (1. / this->pi_ + 1. / g.pi_);
    T new_tau = new_pi * (this->tau_ / this->pi_ + g.tau_ / g.pi_);
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator+=(const Gaussian &g)
  {
    // Add myself and a Gaussian.
    T new_pi   = 1. / (1. / this->pi_ + 1. / g.pi_);
    this->tau_ = new_pi * (this->tau_ / this->pi_ + g.tau_ / g.pi_);
    this->pi_  = new_pi;
    return *this;
  }

  Gaussian operator-() const
  {
    // Invert the mean value.
    return Gaussian(this->pi_, -this->tau_);
  }
  Gaussian operator-(const Gaussian &g) const
  {
    // Subtract two Gaussians.
    T new_pi  = 1. / (1. / this->pi_ + 1. / g.pi_);
    T new_tau = new_pi * (this->tau_ / this->pi_ - g.tau_ / g.pi_);
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator-=(const Gaussian &g)
  {
    // Subtract a Gaussian from myself.
    T new_pi   = 1. / (1. / this->pi_ + 1. / g.pi_);
    this->tau_ = new_pi * (this->tau_ / this->pi_ - g.tau_ / g.pi_);
    this->pi_  = new_pi;
    return *this;
  }

  void set_mu_sigma(T mu, T sigma)
  {
    pi_  = 1 / (sigma * sigma);
    tau_ = mu * pi_;
  }

  T pi() const
  {
    return pi_;
  }
  T tau() const
  {
    return tau_;
  }

  T mu() const
  {
    return tau_ / pi_;
  }
  T sigma() const
  {
    return 1 / sqrt(pi_);
  }

private:
  T pi_  = 0.;
  T tau_ = 0.;
};

template <typename T>
Gaussian<T> operator*(T s, const Gaussian<T> &g)
{
  return g * s;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch