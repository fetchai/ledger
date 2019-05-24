#include <iostream>
#include "core/random/lcg.hpp"
#include "math/tensor.hpp"
#include "math/matrix_operations.hpp"
#include "math/approx_exp.hpp"
#include "vectorise/memory/array.hpp"
#include "vectorise/memory/shared_array.hpp"
#include "vectorise/memory/shared_array.hpp"
#include <chrono>
#include <Eigen/Dense>
using Eigen::MatrixXd;
using Eigen::MatrixXf;
using namespace fetch::random;
using namespace fetch::math;
using namespace std::chrono;

void TestEigenXX()
{
  using Type = float;
  int32_t N = 1000;
  MatrixXf a(N,N), b(N,N), c(N,N);
  LinearCongruentialGenerator lcg;

  for(int32_t i=0; i < N; ++i)
  {
    for(int32_t j=0; j < N; ++j)
    {    
      a(i,j) = static_cast<Type>(lcg.AsDouble());
      b(i,j) = static_cast<Type>(lcg.AsDouble());
    }
  }

  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  for(uint64_t i=0; i < 1; ++i)
  {
    c = a * b ;
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  std::cout << "It took me " << time_span.count() << " seconds.";
  std::cout << std::endl;    
}

void TestFetchXX()
{
  using Type = float;
  uint64_t N = 1000;
  Tensor<Type> a({N,N}), b({N,N}), c({N,N});
  LinearCongruentialGenerator lcg;

  for(uint64_t i=0; i < N; ++i)
  {
    for(uint64_t j=0; j < N; ++j)
    {    
      a(i,j) = static_cast<Type>(lcg.AsDouble());
      b(i,j) = static_cast<Type>(lcg.AsDouble());
    }
  }

  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  for(uint64_t i=0; i < 1; ++i)
  {
    Dot(a,b,c);
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  std::cout << "It took me " << time_span.count() << " seconds.";
  std::cout << std::endl;  
}


/*
using type        = float;
using array_type  = fetch::memory::Array<type>;
using vector_type = typename array_type::VectorRegisterType;


#define TEST_COUNT 3000
void TestEigen(int32_t N)
{
  MatrixXf a(N,N);
  LinearCongruentialGenerator lcg;

  for(int32_t i=0; i < N; ++i)
  {
    for(int32_t j=0; j < N; ++j)
    {    
      a(i,j) = static_cast<float>(lcg.AsDouble());
    }
  }

  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  for(uint64_t x=0; x < TEST_COUNT; ++x)
  {
    for(int32_t i=0; i < N; ++i)
    {
      for(int32_t j=0; j < N; ++j)
      {    
        a(i,j) = exp(a(i,j));
      }
    }
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  std::cout << time_span.count() << " ";
}

void TestEigen2(int32_t N)
{
  MatrixXf a(N,N);
  LinearCongruentialGenerator lcg;
  fetch::math::ApproxExpImplementation<0> fexp;

  for(int32_t i=0; i < N; ++i)
  {
    for(int32_t j=0; j < N; ++j)
    {    
      a(i,j) = static_cast<float>(lcg.AsDouble());
    }
  }

  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  for(uint64_t x=0; x < TEST_COUNT; ++x)
  {
    for(int32_t i=0; i < N; ++i)
    {
      for(int32_t j=0; j < N; ++j)
      {    
        a(i,j) = static_cast<float>(fexp(a(i,j)));
      }
    }
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  std::cout << time_span.count() << " ";
}

void TestEigen3(int32_t N)
{
  MatrixXf a(N,N);
  LinearCongruentialGenerator lcg;
  fetch::math::ApproxExpImplementation<0> fexp;

  for(int32_t i=0; i < N; ++i)
  {
    for(int32_t j=0; j < N; ++j)
    {    
      a(i,j) = static_cast<float>(lcg.AsDouble());
    }
  }

  high_resolution_clock::time_point t1 = high_resolution_clock::now();

  for(uint64_t x=0; x < TEST_COUNT; ++x)
  {
    for(int32_t i=0; i < N; ++i)
    {
      for(int32_t j=0; j < N; ++j)
      {    
        a(i,j) = static_cast<float>(fexp(a(i,j)));
      }
    }
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  std::cout << time_span.count() << " ";
}

void TestFetch(uint64_t N)
{
  Tensor<float> a({N,N});
  LinearCongruentialGenerator lcg;

  for(uint64_t i=0; i < N; ++i)
  {
    for(uint64_t j=0; j < N; ++j)
    {    
      a(i,j) = static_cast<float>(lcg.AsDouble());
    }
  }

  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  for(uint64_t i=0; i < TEST_COUNT; ++i)
  {
    a.data().in_parallel().Apply([](vector_type const &x, vector_type &y) { y = approx_exp(x); }, a.data());
  }

  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  std::cout << time_span.count() << " ";
}
*/
int main()
{
  uint64_t N = 1000;
  Tensor<float> a({N,N}), b({N,N}), c({N,N})  ;
//  DotTranspose(a,b, c);
  TestEigenXX();
  TestFetchXX();
  /*
  for(uint64_t i=0; i < 11; ++i)
  {
    std::cout << i << " ";
    TestEigen(1<<i);
    TestEigen2(1<<i);
    TestFetch(1ull<<i);
    std::cout << std::endl;
  }
  std::cout << std::endl;
  */
  return 0;
}