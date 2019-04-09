#include "math/tensor_operations.hpp"
#include "math/tensor.hpp"
#include <iostream>
#include <typeinfo>
#include "core/assert.hpp"
#include "meta/type_traits.hpp"

template <typename T> struct is_vector { static const bool value = false; };
template <typename T> struct is_vector< std::vector<T> > { static const bool value = true; };

template <typename T, typename R = void>
using IfIsVector = fetch::meta::EnableIf<is_vector<T>::value, R>;

template <typename T, typename R = void>
using IfIsNotVector = fetch::meta::EnableIf<!is_vector<T>::value, R>;


template <typename ScalarType, typename TensorType>
IfIsNotVector<ScalarType, void> _init_nd_tensor(const ScalarType s, TensorType& t, std::vector<typename TensorType::SizeType>& counter, typename TensorType::SizeType){
    for(auto &i: counter){
        std::cout << i;
    }
  t.At(counter) = s;  

}

template <typename VectorType, typename TensorType>
IfIsVector<VectorType, void> _init_nd_tensor(const VectorType& init_vector, TensorType& t, std::vector<typename TensorType::SizeType> counter, typename TensorType::SizeType dim){
  for(u_int64_t i = 0; i < init_vector.size(); i++){
      
    _init_nd_tensor(init_vector[i], t, counter, dim+1);
    counter[dim] += 1;
  }
}

template <typename VectorType, typename TensorType>
void init_nd_tensor(const VectorType &init_vector, TensorType& t){
    std::vector<typename TensorType::SizeType> counter(t.shape().size(), 0);
    typename TensorType::SizeType dim{0};
    _init_nd_tensor(init_vector, t, counter, dim);
}

int main(){

    // fetch::math::Tensor<double> t1(std::vector<uint64_t>({2,3}));
    // fetch::math::Tensor<double> t2(std::vector<uint64_t>({2,2}));
    // fetch::math::Tensor<double> t3(std::vector<uint64_t>({2,2}));

    // std::vector<std::vector<double>> lv1{{1,2, 5},{3,4, 6}};
    // std::vector<std::vector<double>> lv2{{10,20},{30,40}};
    // std::vector<std::vector<double>> lv3{{100,200},{300,400}};

    // init_nd_tensor(lv1, t1);
    // init_nd_tensor(lv2, t2);
    // init_nd_tensor(lv3, t3);


    // std::vector<fetch::math::Tensor<double>> tensors{t1, t2, t3};
    // auto d = fetch::math::concatenate(tensors, 1);


}