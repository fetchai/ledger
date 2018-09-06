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

#include <type_traits>
#include <cstdint>


namespace fetch {
namespace crypto {
namespace detail {


template<typename NUMBER_TYPE, std::size_t SIZE_IN_BYTES = sizeof(NUMBER_TYPE)>
struct FNVConfig
{
  static_assert(std::is_integral<NUMBER_TYPE>::value && sizeof(NUMBER_TYPE) >= SIZE_IN_BYTES, "Provided SIZE_IN_BYTES parameter value must be smaller or equal to `sizeof(...)` of provided integral type NUMBER_TYPE type parameter.");

  using number_type = NUMBER_TYPE;
  static constexpr uint8_t size_in_bytes = SIZE_IN_BYTES;
  static number_type const prime;
  static number_type const offset;
};

template<typename NUMBER_TYPE, std::size_t SIZE_IN_BYTES> constexpr uint8_t FNVConfig<NUMBER_TYPE, SIZE_IN_BYTES>::size_in_bytes;

template<> FNVConfig<uint32_t>::number_type const FNVConfig<uint32_t>::prime;
template<> FNVConfig<uint32_t>::number_type const FNVConfig<uint32_t>::offset;

template<> FNVConfig<uint64_t>::number_type const FNVConfig<uint64_t>::prime;
template<> FNVConfig<uint64_t>::number_type const FNVConfig<uint64_t>::offset;


enum class eFnvAlgorithm : uint8_t
{
  fnv1,
  fnv1a,
  fnv0_deprecated
};


template<typename FNV_CONFIG = FNVConfig<std::size_t>, eFnvAlgorithm ALGORITHM = eFnvAlgorithm::fnv1a>
struct FNVAlgorithm
{
  static void update(typename FNV_CONFIG::number_type& context, uint8_t const *data_to_hash, std::size_t const &size);
  static void reset(typename FNV_CONFIG::number_type& context);
};

template<typename FNV_CONFIG>
struct FNVAlgorithm<FNV_CONFIG, eFnvAlgorithm::fnv1a>
{
  static void update(typename FNV_CONFIG::number_type& context, uint8_t const *data_to_hash, std::size_t const &size)
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      context ^= data_to_hash[i];
      context *= FNV_CONFIG::prime;
    }
  }

  static void reset(typename FNV_CONFIG::number_type& context)
  {
    context = FNV_CONFIG::offset;
  }
};

template<typename FNV_CONFIG>
struct FNVAlgorithm<FNV_CONFIG, eFnvAlgorithm::fnv1>
{
  static void update(typename FNV_CONFIG::number_type& context, uint8_t const *data_to_hash, std::size_t const &size)
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      context *= FNV_CONFIG::prime;
      context ^= data_to_hash[i];
    }
  }

  static void reset(typename FNV_CONFIG::number_type& context)
  {
    context = FNV_CONFIG::offset;
  }
};

template<typename FNV_CONFIG>
struct FNVAlgorithm<FNV_CONFIG, eFnvAlgorithm::fnv0_deprecated>
{
  static void update(typename FNV_CONFIG::number_type& context, uint8_t const *data_to_hash, std::size_t const &size)
  {
    FNVAlgorithm<FNV_CONFIG, eFnvAlgorithm::fnv1>::update(context, data_to_hash, size);
  }

  static void reset(typename FNV_CONFIG::number_type& context)
  {
    context = 0;
  }
};


template<typename T=std::size_t, eFnvAlgorithm ALGORITHM=eFnvAlgorithm::fnv1a, std::size_t SIZE_IN_BYTES = FNVConfig<T>::size_in_bytes>
class FNV : public FNVConfig<T, SIZE_IN_BYTES>
{
public:
  using base_type = FNVConfig<T, SIZE_IN_BYTES>;
  using number_type = typename base_type::number_type;
  static constexpr eFnvAlgorithm algorithm = ALGORITHM;

private:
  number_type context_;

public:

  FNV()
  {
    reset();
  }

  void update(uint8_t const *data_to_hash, std::size_t const &size)
  {
    FNVAlgorithm<base_type, ALGORITHM>::update(context_, data_to_hash, size);
  }

  void reset()
  {
    FNVAlgorithm<base_type, ALGORITHM>::reset(context_);
  }

  number_type const & context() const
  {
    return context_;
  }
};

template<typename T, eFnvAlgorithm ALGORITHM, std::size_t SIZE_IN_BYTES>
constexpr eFnvAlgorithm FNV<T, ALGORITHM, SIZE_IN_BYTES>::algorithm;

using FNV1a = FNV<std::size_t, detail::eFnvAlgorithm::fnv1a>;
using FNV1 = FNV<std::size_t, detail::eFnvAlgorithm::fnv1>;

} // namespace detail
} // namespace crypto
} // namespace fetch
