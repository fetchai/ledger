#include <type_traits>
#include <vector>
#include <iostream> 
#include "math/tensor.hpp"
#include "meta/type_traits.hpp"

// template <bool C, typename R = void>
// using EnableIf = typename std::enable_if<C, R>::type;

using namespace fetch::meta;

// template <typename T, typename R = void>
// using IfIsArithmetic = EnableIf<std::is_arithmetic<T>::value, R>;

template <typename T> struct is_vector { static const bool value = false; };
template <typename T> struct is_vector< std::vector<T> > { static const bool value = true; };

template <typename T, typename R = void>
using IfIsVector = fetch::meta::EnableIf<is_vector<T>::value, R>;

template <typename T, typename R = void>
using IfIsNotVector = fetch::meta::EnableIf<!is_vector<T>::value, R>;


template <typename S>
IfIsArithmetic<S, void> Add(S const &scalar1, S const &scalar2, S &ret)
{
 ret = scalar1 + scalar2;
}


template <typename S>
IfIsArithmetic<S, S> Add(S const &scalar1, S const &scalar2)
{
 S ret;
 Add(scalar1, scalar2, ret);
 return ret;
}


template <typename ScalarType, typename TensorType>
IfIsNotVector<ScalarType, void> _init_nd_tensor(ScalarType s, TensorType& t, std::vector<typename TensorType::SizeType> counter, typename TensorType::SizeType dim){
  t.At(counter) = s;  
  counter[dim] += 1;
}

template <typename VectorType, typename TensorType>
IfIsVector<VectorType, void> _init_nd_tensor(VectorType init_vector, TensorType& t, std::vector<typename TensorType::SizeType> counter, typename TensorType::SizeType dim){
  //std::cout << V.size() << std::endl;
  for(auto &e: init_vector){
    _init_nd_tensor(e, t, counter, dim+1);
    counter[dim] += 1;
  }
}

template <typename VectorType, typename TensorType>
void init_nd_tensor(VectorType& init_vector, TensorType& t){
    _init_nd_tensor(init_vector, t, std::vector<typename TensorType::SizeType>(t.shape().size(),0), typename TensorType::SizeType{0});
}

using T = float;

int main(){
  T a = 3.1;
  T b = 3.5;
  T c{Add(a, b)};

  std::vector<uint64_t> counter(3,0);
  std::vector<std::vector<std::vector<double>>> vv{{{1,2},{3,4}},{{5,6},{7,8}}};
  fetch::math::Tensor<double> tt(std::vector<uint64_t>({2,2,2}));
  //std::cout << tt.At(counter);
  int dim = 0;
  //Blub(vv, tt, counter, dim);
  init_nd_tensor(vv, tt);
  std::cout << tt.At({1,1,0});
}

// template<bool, typename _Tp = void>
// struct enable_if
// { };

// // Partial specialization for true.
// template<typename _Tp>
// struct enable_if<true, _Tp>
// { typedef _Tp type; };

