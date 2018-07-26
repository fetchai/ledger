#define FETCH_TESTING_ENABLED
#include "vectorise/memory/shared_array.hpp"
#include "iostream"
#include "random/lcg.hpp"
using namespace fetch::memory;

typedef uint64_t               data_type;
typedef SharedArray<data_type> array_type;

void test_set_get()
{
  static fetch::random::LinearCongruentialGenerator lcg1, lcg2;
  lcg1.Reset();
  lcg2.Reset();
  std::size_t N = lcg1() % 100000;
  lcg2();

  array_type array(N), other;
  for (std::size_t i = 0; i < N; ++i)
  {
    array[i] = lcg1();
  }

  for (std::size_t i = 0; i < N; ++i)
  {
    if (array[i] != lcg2())
    {
      std::cout << "1: memory doesn't store what it is supposed to"
                << std::endl;
      exit(-1);
    }
  }

  other = array;
  lcg2.Reset();
  lcg2();
  for (std::size_t i = 0; i < N; ++i)
  {
    if (other[i] != lcg2())
    {
      std::cout << "2: memory doesn't store what it is supposed to"
                << std::endl;
      exit(-1);
    }
  }

  array_type yao(other);
  lcg2.Reset();
  lcg2();
  for (std::size_t i = 0; i < N; ++i)
  {
    if (yao[i] != lcg2())
    {
      std::cout << "3: memory doesn't store what it is supposed to"
                << std::endl;
      exit(-1);
    }
  }

  array = array;
  /*
  if(array.reference_count() != 3) {
    std::cout << "expected array to be referenced exactly 3 times";
    std::cout << "but is referenced " << array.reference_count() <<  std::endl;
  }

  if(testing::total_shared_objects != 1) {
    std::cout << "expected exactly 1 object but " <<
  testing::total_shared_objects;
    std::cout << "found" << std::endl;
  }
  */
  lcg1.Seed(lcg1());
  lcg2.Seed(lcg2());
}

int main()
{
  for (std::size_t i = 0; i < 100; ++i)
  {
    test_set_get();
  }
  return 0;
}
