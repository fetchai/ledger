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


#include <cmath>

namespace fetch {
namespace math {
namespace statistics {
namespace normal {

namespace helper {
  template <typename T>
  constexpr T pi()
  {
    return std::atan(static_cast<T>(1))*4;
  }
}

/**
 * Probability density of normal distribution with mu and std.
 */
template <typename T>
inline T pdf(T const &m, T const &s, T const &x)
{
  return std::exp(-(x-m)*(x-m)/(2.*s*s))/(s*sqrt(2.*helper::pi<T>()));
}

/**
 * Cumulative distribution function of normal distribution.
 */
template <typename T>
inline T cdf(T const &m, T const &s, T const &x)
{
  return 0.5*std::erfc(-(x-m)/(s*sqrt(2.)));
}

/**
 * This function implements the Pade approximation for the inverse of the erf function.
 * It is a good approximation in [-0.9,0.9].
 */
template <typename T>
inline T erf_inv(T const &x)
{
  return (std::sqrt(helper::pi<T>())/2.) * x 
    * (1-4397*helper::pi<T>()*std::pow(x,2)/17352.+111547*std::pow(helper::pi<T>(),2)*std::pow(x,4)/14575680.)
    / (1-5843*helper::pi<T>()*std::pow(x,2)/17352.+20533 *std::pow(helper::pi<T>(),2)*std::pow(x,4)/971712.  );
}

template <typename T>
inline T erfc_inv(T const &z)
{
  return erf_inv(1-z);
}

template <typename T>
inline T quantile(T const &m, T const &s, T const &p)
{
  return m-s*std::sqrt(2)*erfc_inv(2*p);
}

} //namespace normal

/**
 * Gaussian class implements various operations with Gaussian distributions.
 */
template <typename T>
class Gaussian {
public:
  Gaussian(){};
  Gaussian(T pi, T tau): _pi(pi), _tau(tau) {
  }

  static Gaussian ClassicForm(T mu, T sigma)
  {
    Gaussian g;
    g.set_mu_sigma(mu, sigma);
    return g;
  }

  Gaussian operator*(const Gaussian& g) const 
  {
    // Multiply two Gaussians.
    T new_pi = this->_pi + g._pi;
    T new_tau = this->_tau + g._tau;
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator*=(const Gaussian& g)
  {
    // Multiply myself and another Gaussian.
    this->_pi += g._pi;
    this->_tau += g._tau;
    return *this;
  }

  Gaussian operator*(T s) const
  {
    // Multiply by scalar. This action increases only the variance.
    T k = 1 + this->_pi*s*s;
    T new_pi = this->_pi/k;
    T new_tau = this->_tau/k;
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator*=(T s)
  {
    // Multiply myself by scalar. This action increases only the variance.
    T k = 1 + this->_pi*s*s;
    this->_pi /= k;
    this->_tau /= k;
    return *this;
  }

  Gaussian operator/(const Gaussian& g) const
  {
    // Divide two Gaussians.
    T new_pi = this->_pi - g._pi;
    T new_tau = this->_tau - g._tau;
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator/=(const Gaussian& g)
  {
    // Divide by Gaussian.
    this->_pi -= g._pi;
    this->_tau -= g._tau;
    return *this;
  }

  Gaussian operator+(const Gaussian& g) const
  {
    // Add two Gaussians.
    T new_pi = 1./(1./this->_pi + 1./g._pi);
    T new_tau = new_pi*(this->_tau/this->_pi + g._tau/g._pi);
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator+=(const Gaussian& g)
  {
    // Add myslef and a Gaussian.
    T new_pi = 1./(1./this->_pi + 1./g._pi);
    this->_tau = new_pi*(this->_tau/this->_pi + g._tau/g._pi);
    this->_pi = new_pi;
    return *this;
  }

  Gaussian operator-() const
  {
    // Invert the mean value.
    return Gaussian(this->_pi, -this->_tau);
  }
  Gaussian operator-(const Gaussian& g) const
  {
    // Subtract two Gaussians.
    T new_pi = 1./(1./this->_pi + 1./g._pi);
    T new_tau = new_pi*(this->_tau/this->_pi - g._tau/g._pi);
    return Gaussian(new_pi, new_tau);
  }
  Gaussian operator-=(const Gaussian& g)
  {
    // Subtract a Gaussian from myslef.
    T new_pi = 1./(1./this->_pi + 1./g._pi);
    this->_tau = new_pi*(this->_tau/this->_pi - g._tau/g._pi);
    this->_pi = new_pi;
    return *this;
  }

  void set_mu_sigma(T mu, T sigma)
  {
    _pi = 1/(sigma*sigma);
    _tau = mu*_pi;
  }

  T pi() const
  {
    return _pi;
  }
  T tau() const
  {
    return _tau;
  }

  T mu() const
  {
    return _tau/_pi;
  }
  T sigma() const
  {
    return 1/sqrt(_pi);
  }
  
private:
    T _pi = 0.;
    T _tau = 0.;

};

template <typename T>
Gaussian<T> operator*(T s, const Gaussian<T> &g)
{
  return g*s;
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch