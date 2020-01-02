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

#include <cstdint>
#include <type_traits>

namespace fetch {
namespace crypto {
namespace detail {

struct FNVConfigInvalid
{
  static constexpr std::size_t SIZE_IN_BYTES = 0;
};

template <typename NumberTypeParam, std::size_t SIB = sizeof(NumberTypeParam),
          typename FROM = FNVConfigInvalid>
struct FNVConfig
{
  static_assert(std::is_integral<NumberTypeParam>::value && sizeof(NumberTypeParam) >= SIB,
                "Provided SIZE_IN_BYTES parameter value must be smaller or equal to `sizeof(...)` "
                "of provided integral type NumberType type parameter.");

  static_assert(std::is_same<FNVConfigInvalid, FROM>::value || FROM::SIZE_IN_BYTES == SIB, "");

  using NumberType                           = NumberTypeParam;
  using FromType                             = FROM;
  static constexpr std::size_t SIZE_IN_BYTES = SIB;
  static NumberType const      prime;
  static NumberType const      offset;
};

template <typename NumberType, std::size_t SIZE_IN_BYTES, typename FROM>
constexpr std::size_t FNVConfig<NumberType, SIZE_IN_BYTES, FROM>::SIZE_IN_BYTES;

template <>
FNVConfig<uint32_t>::NumberType const FNVConfig<uint32_t>::prime;
template <>
FNVConfig<uint32_t>::NumberType const FNVConfig<uint32_t>::offset;

template <>
FNVConfig<uint64_t>::NumberType const FNVConfig<uint64_t>::prime;
template <>
FNVConfig<uint64_t>::NumberType const FNVConfig<uint64_t>::offset;

template <typename NumberType, std::size_t SIZE_IN_BYTES, typename FROM>
typename FNVConfig<NumberType, SIZE_IN_BYTES, FROM>::NumberType const
    FNVConfig<NumberType, SIZE_IN_BYTES, FROM>::prime = FROM::prime;

template <typename NumberType, std::size_t SIZE_IN_BYTES, typename FROM>
typename FNVConfig<NumberType, SIZE_IN_BYTES, FROM>::NumberType const
    FNVConfig<NumberType, SIZE_IN_BYTES, FROM>::offset = FROM::offset;

enum class eFnvAlgorithm : uint8_t
{
  fnv1,
  fnv1a,
  fnv0_deprecated
};

using FNVConfig_size_t =
    FNVConfig<std::size_t, sizeof(std::size_t),
              std::conditional<sizeof(std::size_t) == FNVConfig<uint64_t>::SIZE_IN_BYTES,
                               FNVConfig<uint64_t>, FNVConfig<uint32_t>>::type>;

template <typename FNV_CONFIG = FNVConfig_size_t, eFnvAlgorithm ALGORITHM = eFnvAlgorithm::fnv1a>
struct FNVAlgorithm
{
  static void update(typename FNV_CONFIG::NumberType &context, uint8_t const *data_to_hash,
                     std::size_t size);
  static void reset(typename FNV_CONFIG::NumberType &context);
};

template <typename FNV_CONFIG>
struct FNVAlgorithm<FNV_CONFIG, eFnvAlgorithm::fnv1a>
{
  static void update(typename FNV_CONFIG::NumberType &context, uint8_t const *data_to_hash,
                     std::size_t size)
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      context ^= data_to_hash[i];
      context *= FNV_CONFIG::prime;
    }
  }

  static void reset(typename FNV_CONFIG::NumberType &context)
  {
    context = FNV_CONFIG::offset;
  }
};

template <typename FNV_CONFIG>
struct FNVAlgorithm<FNV_CONFIG, eFnvAlgorithm::fnv1>
{
  static void update(typename FNV_CONFIG::NumberType &context, uint8_t const *data_to_hash,
                     std::size_t size)
  {
    for (std::size_t i = 0; i < size; ++i)
    {
      context *= FNV_CONFIG::prime;
      context ^= data_to_hash[i];
    }
  }

  static void reset(typename FNV_CONFIG::NumberType &context)
  {
    context = FNV_CONFIG::offset;
  }
};

template <typename FNV_CONFIG>
struct FNVAlgorithm<FNV_CONFIG, eFnvAlgorithm::fnv0_deprecated>
{
  static void update(typename FNV_CONFIG::NumberType &context, uint8_t const *data_to_hash,
                     std::size_t size)
  {
    FNVAlgorithm<FNV_CONFIG, eFnvAlgorithm::fnv1>::update(context, data_to_hash, size);
  }

  static void reset(typename FNV_CONFIG::NumberType &context)
  {
    context = 0;
  }
};

template <typename FNV_CONFIG = FNVConfig_size_t, eFnvAlgorithm ALGORITHM = eFnvAlgorithm::fnv1a>
class FNV : public FNV_CONFIG
{
public:
  using BaseType                           = FNV_CONFIG;
  using NumberType                         = typename BaseType::NumberType;
  static constexpr eFnvAlgorithm algorithm = ALGORITHM;

private:
  NumberType context_;

public:
  FNV()
  {
    reset();
  }

  void update(uint8_t const *data_to_hash, std::size_t size)
  {
    FNVAlgorithm<BaseType, ALGORITHM>::update(context_, data_to_hash, size);
  }

  void reset()
  {
    FNVAlgorithm<BaseType, ALGORITHM>::reset(context_);
  }

  NumberType const &context() const
  {
    return context_;
  }
};

template <typename FNV_CONFIG, eFnvAlgorithm ALGORITHM>
constexpr eFnvAlgorithm FNV<FNV_CONFIG, ALGORITHM>::algorithm;

using FNV1a = FNV<FNVConfig_size_t, detail::eFnvAlgorithm::fnv1a>;
using FNV1  = FNV<FNVConfig_size_t, detail::eFnvAlgorithm::fnv1>;

}  // namespace detail
}  // namespace crypto
}  // namespace fetch
