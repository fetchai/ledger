#pragma once

namespace fetch
{
namespace math
{

namespace details 
{
static const float  DEFAULT_RELATIVE_FLT_TOLERANCE = 1e-5f;
static const float  DEFAULT_ABSOLUTE_FLT_TOLERANCE = 1e-5f;
static const double DEFAULT_RELATIVE_DBL_TOLERANCE = 1e-14;
static const double DEFAULT_ABSOLUTE_DBL_TOLERANCE = 1e-14;
}

inline float Tolerance(float const &a, float const &b, float const &rel_tol = details::DEFAULT_RELATIVE_FLT_TOLERANCE, float const &abs_tol = details::DEFAULT_ABSOLUTE_FLT_TOLERANCE)
{
  return std::max(rel_tol * std::max(fabsf(a), fabsf(b)), abs_tol);
}
inline double Tolerance(double const &a, double const &b, double const &rel_tol = details::DEFAULT_RELATIVE_DBL_TOLERANCE, double const &abs_tol = details::DEFAULT_ABSOLUTE_DBL_TOLERANCE)
{
  return std::max(rel_tol * std::max(fabs(a), fabs(b)), abs_tol);
}

inline bool IsZero(float const &x, float const &abs_tol = details::DEFAULT_ABSOLUTE_FLT_TOLERANCE) 
{ 
    // Note equality is important - otherwise you can have a number that is neither zero nor non-zero
  return fabsf(x) <= abs_tol; 
}

inline bool IsZero(double const &x, double const &abs_tol = details::DEFAULT_ABSOLUTE_DBL_TOLERANCE) 
{ 
  // Note equality is important - otherwise you can have a number that is neither zero nor non-zero
  return fabs(x) <= abs_tol; 
}

template <typename T>
bool IsZero(T const &x)
{
  return x == 0;
}

inline bool IsNonZero(float const &x, float const &abs_tol = details::DEFAULT_ABSOLUTE_FLT_TOLERANCE) 
{ 
  return fabsf(x) > abs_tol;
}

inline bool IsNonZero(double const &x, double const &abs_tol = details::DEFAULT_ABSOLUTE_DBL_TOLERANCE) 
{ 
  return fabs(x) > abs_tol;
}

template <typename T>
bool IsNonZero(T const &x)
{
  return x != 0;
}

inline bool IsEqual(float const &a, float const &b) 
{ 
  return fabsf(a - b) < Tolerance(a, b); 
}

inline bool IsEqual(double const &a, double const &b) 
{ 
  return fabs(a - b) < Tolerance(a, b); 
}

template <typename T>
bool IsEqual(T const &a, T const &b)
{
  return a == b;
}

inline bool IsNotEqual(float const &a, float const &b) 
{ 
  return fabsf(a - b) > Tolerance(a, b); 
}

inline bool IsNotEqual(double const &a, double const &b) 
{ 
  return fabs(a - b) > Tolerance(a, b); 
}

template <typename T>
bool IsNotEqual(T const &a, T const &b)
{
  return a != b;
}

inline bool IsLessThan(float const &a, float const &b) 
{ 
  return a < b - Tolerance(a, b); 
}

inline bool IsLessThan(double const &a, double const &b) 
{ 
  return a < b - Tolerance(a, b); 
}

template <typename T>
bool IsLessThan(const T a, const T b)
{
  return a < b;
}

inline bool IsLessThanOrEqual(float const &a, float const &b) 
{ 
  return a < b + Tolerance(a, b); 
}

inline bool IsLessThanOrEqual(double const &a, double const &b) 
{ 
  return a < b + Tolerance(a, b); 
}

template <typename T>
bool IsLessThanOrEqual(const T a, const T b)
{
  return a <= b;
}

inline bool IsGreaterThan(float const &a, const float b) 
{ 
  return a > b + Tolerance(a, b); 
}

inline bool IsGreaterThan(double const &a, const double b) 
{ 
  return a > b + Tolerance(a, b); 
}

template <typename T>
bool IsGreaterThan(const T a, const T b)
{
  return a > b;
}

inline bool IsGreaterThanOrEqual(float const &a, float const &b) 
{ 
  return a > b - Tolerance(a, b); 
}

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