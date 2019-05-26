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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/arch/sse/info.hpp"
#include "vectorise/arch/sse/register_int32.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <emmintrin.h>
#include <immintrin.h>
#include <smmintrin.h>

#include <iostream>

namespace fetch {
namespace vectorize {

template <>
class VectorRegister<fixed_point::FixedPoint<32,32>, 128>
{
public:
  using type             = fixed_point::FixedPoint<32,32>;
  using mm_register_type = __m128;

  enum
  {
    E_VECTOR_SIZE   = 128,
    E_REGISTER_SIZE = sizeof(mm_register_type),
    E_BLOCK_COUNT   = E_REGISTER_SIZE / sizeof(type)
  };

  static_assert((E_BLOCK_COUNT * sizeof(type)) == E_REGISTER_SIZE,
                "type cannot be contained in the given register size.");

  VectorRegister() = default;
  VectorRegister(type const * /*d*/)
  {
    throw std::runtime_error("load pointer not implemented");
  }
  VectorRegister(mm_register_type const &d)
    : data_(d)
  {}
  VectorRegister(mm_register_type &&d)
    : data_(d)
  {}
  VectorRegister(type const & /*c*/)
  {
    throw std::runtime_error("load data not implemented");
  }

  explicit operator mm_register_type()
  {
    return data_;
  }

  void Store(type * /*ptr*/) const
  {
    throw std::runtime_error("store pointer not implemented");
  }

  void Stream(type * /*ptr*/) const
  {
    throw std::runtime_error("stream pointer not implemented");
  }

  mm_register_type const &data() const
  {
    return data_;
  }
  mm_register_type &data()
  {
    return data_;
  }

private:
  mm_register_type data_;
};

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator-(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}


inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator*(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator-(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator/(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator+(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}


inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator==(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator!=(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator>=(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator>(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator<=(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> operator<(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/, VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*y*/)
{
  throw std::runtime_error("operator not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> vector_zero_below_element(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const &/*a*/,
                                                            int const &                       /*n*/)
{
  throw std::runtime_error("vector_zero_below_element not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> vector_zero_above_element(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const &/*a*/,
                                                            int const &                       /*n*/)
{
  throw std::runtime_error("vector_zero_above_element not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> shift_elements_left(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/)
{
  throw std::runtime_error("shift_elements_left not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline VectorRegister<fixed_point::FixedPoint<32,32>, 128> shift_elements_right(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/)
{
  throw std::runtime_error("shift_elements_right not implemented.");
  return VectorRegister<fixed_point::FixedPoint<32,32>, 128>(  fixed_point::FixedPoint<32,32>{} );
}

inline fixed_point::FixedPoint<32,32> first_element(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/)
{
  throw std::runtime_error("first_element not implemented.");
  return fixed_point::FixedPoint<32,32>{};
}

inline fixed_point::FixedPoint<32,32> reduce(VectorRegister<fixed_point::FixedPoint<32,32>, 128> const & /*x*/)
{
  throw std::runtime_error("reduce not implemented."); 
  return fixed_point::FixedPoint<32,32>{}; 
}




}
}