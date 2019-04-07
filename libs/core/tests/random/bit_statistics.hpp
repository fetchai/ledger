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

#include "core/random/lfg.hpp"

#include <cmath>
#include <iostream>
#include <vector>

template <typename T = fetch::random::LaggedFibonacciGenerator<>>
class BitStatistics
{
public:
  enum
  {
    E_BIT_COUNT = 8 * sizeof(typename T::random_type)
  };
  BitStatistics()
  {
    stats_.resize(E_BIT_COUNT);
    Reset();
  }

  void operator()()
  {
    uint64_t s = generator_();
    for (std::size_t i = 0; i < E_BIT_COUNT; ++i)
    {
      stats_[i] += uint32_t((s >> i) & 1);
    }
    ++counter_;
  }

  void Repeat(std::size_t N)
  {
    for (std::size_t i = 0; i < N; ++i)
    {
      this->operator()();
    }
  }

  void Reset()
  {
    for (auto &a : stats_)
    {
      a = 0;
    }
    counter_ = 0;
  }

  std::vector<double> GetProbabilities()
  {
    std::vector<double> ret;
    ret.resize(E_BIT_COUNT);
    double rec = 1. / double(counter_);
    for (std::size_t i = 0; i < E_BIT_COUNT; ++i)
    {
      ret[i] = stats_[i] * rec;
    }
    return ret;
  }

  bool TestAccuracy(std::size_t N, double tol)
  {
    Reset();
    Repeat(N);
    for (auto &a : GetProbabilities())
    {
      if (fabs(a - 0.5) > tol)
      {
        std::cout << "bit p=0.5 deviation of more than " << tol << std::endl;
        return false;
      }
    }
    return true;
  }

  std::vector<uint32_t> const &stats() const
  {
    return stats_;
  }

private:
  std::vector<uint32_t> stats_;
  T                     generator_;
  std::size_t           counter_ = 0;
};
