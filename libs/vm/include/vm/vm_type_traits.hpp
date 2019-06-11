#pragma once

namespace fetch
{
namespace vm
{
template <typename T, typename = void>
struct IsPrimitive : std::false_type
{
};
template <typename T>
struct IsPrimitive<
    T, std::enable_if_t<type_util::IsAnyOfV<T, void, bool, int8_t, uint8_t, int16_t, uint16_t,
                                            int32_t, uint32_t, int64_t, uint64_t, float, double>>>
  : std::true_type
{
};

template <typename T, typename R = void>
using IfIsPrimitive = typename std::enable_if<IsPrimitive<std::decay_t<T>>::value, R>::type;

}
}