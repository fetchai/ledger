#pragma once

namespace fetch
{
namespace math
{

static const float  RELATIVE_FLT_TOLERANCE = 1e-5f;
static const float  ABSOLUTE_FLT_TOLERANCE = 1e-5f;
static const double RELATIVE_DBL_TOLERANCE = 1e-14;
static const double ABSOLUTE_DBL_TOLERANCE = 1e-14;

inline float Tolerance(float const &a, float const &b)
{
  return std::max(RELATIVE_FLT_TOLERANCE * std::max(fabsf(a), fabsf(b)), ABSOLUTE_FLT_TOLERANCE);
}
inline double Tolerance(double const &a, double const &b)
{
  return std::max(RELATIVE_DBL_TOLERANCE * std::max(fabs(a), fabs(b)), ABSOLUTE_DBL_TOLERANCE);
}

inline bool IsZero(float const &x) { return fabsf(x) < ABSOLUTE_FLT_TOLERANCE; }
inline bool IsZero(double const &x) { return fabs(x) < ABSOLUTE_DBL_TOLERANCE; }
template <typename T>
bool IsZero(T const &x)
{
  return x == 0;
}

inline bool IsNonZero(float const &x) { return fabsf(x) > ABSOLUTE_FLT_TOLERANCE; }
inline bool IsNonZero(double const &x) { return fabs(x) > ABSOLUTE_DBL_TOLERANCE; }

template <typename T>
bool IsNonZero(T const &x)
{
  return x != 0;
}

inline bool IsEqual(float const &a, float const &b) { return fabsf(a - b) < Tolerance(a, b); }
inline bool IsEqual(double const &a, double const &b) { return fabs(a - b) < Tolerance(a, b); }
template <typename T>
bool IsEqual(T const &a, T const &b)
{
  return a == b;
}

inline bool IsNotEqual(float const &a, float const &b) { return fabsf(a - b) > Tolerance(a, b); }
inline bool IsNotEqual(double const &a, double const &b) { return fabs(a - b) > Tolerance(a, b); }
template <typename T>
bool IsNotEqual(T const &a, T const &b)
{
  return a != b;
}

inline bool IsLessThan(float const &a, float const &b) { return a < b - Tolerance(a, b); }
inline bool IsLessThan(double const &a, double const &b) { return a < b - Tolerance(a, b); }
template <typename T>
bool IsLessThan(const T a, const T b)
{
  return a < b;
}

inline bool IsLessThanOrEqual(float const &a, float const &b) { return a < b + Tolerance(a, b); }
inline bool IsLessThanOrEqual(double const &a, double const &b) { return a < b + Tolerance(a, b); }
template <typename T>
bool IsLessThanOrEqual(const T a, const T b)
{
  return a <= b;
}

inline bool IsGreaterThan(float const &a, const float b) { return a > b + Tolerance(a, b); }
inline bool IsGreaterThan(double const &a, const double b) { return a > b + Tolerance(a, b); }
template <typename T>
bool IsGreaterThan(const T a, const T b)
{
  return a > b;
}

inline bool IsGreaterThanOrEqual(float const &a, float const &b) { return a > b - Tolerance(a, b); }
inline bool IsGreaterThanOrEqual(double const &a, double const &b)
{
  return a > b - Tolerance(a, b);
}
template <typename T>
bool IsGreaterThanOrEqual(const T a, const T b)
{
  return a >= b;
}

}
}