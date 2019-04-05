#include "math/tensor_operations.hpp"
#include "math/tensor.hpp"
#include <iostream>
#include <typeinfo>
#include "core/assert.hpp"

using namespace fetch::math;

template<typename T_tens, typename T_vec>
void init_2d_tensor(T_tens& tens, T_vec& values_list){
    for (int i = 0; i != values_list.size(); i++){
        for (int j = 0; j!= values_list[i].size(); j++){
            tens.At({i,j}) = values_list[i][j];
        }
    }
}

// template <typename T>
// void print_3d_tensor(T& tens){
//     auto shape = tens.shape();
//     for (int i = 0; i != shape[0]; i++){
//         for (int j = 0; j != shape[1]; j++){
//             for (int k = 0; k != shape[2]; k++){
//                 std::cout << i << "\t" << j << "\t" << k << "\t" << tens.At({i,j,k}) << std::endl;
//             }
//         }
//     }

// }

std::vector<uint64_t> _get_cumsum(std::vector<uint64_t> inp){
    std::vector<uint64_t> result(inp.size());
    result[0] = inp[0];
    for(uint64_t i = 1; i < result.size(); i++){
        result[i] = result[i-1] + inp[i];
    }
    return result;
}

uint64_t _get_array_number(uint64_t pos, std::vector<uint64_t> array_sizes_cumsum){
    for(uint64_t i = 0; i < array_sizes_cumsum.size(); i++){
        if(pos < array_sizes_cumsum[i]){
            return i;
        };
    }
}

// template <typename T>
// void _assert_concat_tensor_shapes(std::vector<T> tensors){
//     std::vector<uint64_t> n_dims(tensors.size());

//     for(uint64_t i = 0; i < tensors.size(); i++){
//         n_dims[i] = tensors[i].shape().size;
//     }

//     if (std::count(std::begin(numbers), std::end(numbers), numbers.front()) != numbers.size())
//     {
//         fetch::assert::ASSERT;
//     }

// }

template <typename T>
std::vector<uint64_t> _infer_shape_of_concat_tensors(std::vector<T> tensors, uint64_t axis){
    // asserts!

    std::vector<uint64_t> final_shape(tensors[0].shape().size());
    
    for(uint64_t i = 0; i < tensors[0].shape().size(); i++){
        if(i!=axis){
            final_shape[i] = tensors[0].shape()[i];
        }
        else{
            for(uint64_t j=0; j < tensors.size(); j++){
                final_shape[i] += tensors[j].shape()[i];
            }
        }
    } 

    return final_shape;
}

template<typename T>
std::vector<uint64_t> _get_dims_along_ax_cumsummed(std::vector<T> tensors, uint64_t axis){
    std::vector<uint64_t> dim_along_concat_ax(tensors.size());

    for(uint64_t i = 0; i < tensors.size(); i++){
        dim_along_concat_ax[i] = tensors[i].shape()[axis];
    }

    std::vector<uint64_t>dim_along_concat_ax_csummed{_get_cumsum(dim_along_concat_ax)};

    return dim_along_concat_ax_csummed;
}

template<typename T>
T concat_along_axis(std::vector<T> tensors, uint64_t axis){
    // assert dims the same    
    // assert dims 

    std::vector<uint64_t> final_shape{_infer_shape_of_concat_tensors<T>(tensors, axis)};
    T new_tensor(final_shape);
    std::vector<uint64_t> dim_along_concat_ax_csummed{_get_dims_along_ax_cumsummed<T>(tensors, axis)};

    uint64_t pos_in_n_arr, n_arr; 
    if(axis==0){
        for(uint64_t i = 0; i < final_shape[0]; i++){
            for(uint64_t j = 0; j < final_shape[1]; j++){
                n_arr = _get_array_number(i, dim_along_concat_ax_csummed);
                if(i >= dim_along_concat_ax_csummed[0]){
                    pos_in_n_arr = i - dim_along_concat_ax_csummed[n_arr-1];
                }
                else{
                    pos_in_n_arr = i;
                }
                new_tensor.At({i,j}) = tensors[n_arr].At({pos_in_n_arr, j});
            }
        }
    }
    else if(axis==1){
        for(uint64_t i = 0; i < final_shape[0]; i++){
            for(uint64_t j = 0; j < final_shape[1]; j++){
                n_arr = _get_array_number(j, dim_along_concat_ax_csummed);
                if(j >= dim_along_concat_ax_csummed[0]){
                    pos_in_n_arr = j - dim_along_concat_ax_csummed[n_arr-1];
                }
                else{
                    pos_in_n_arr = j;
                }
                new_tensor.At({i,j}) = tensors[n_arr].At({i, pos_in_n_arr});
            }
        }
    }

    return new_tensor;
}

int main(){

    auto b = _get_cumsum(std::vector<uint64_t>{3,6,8,2});
    auto c = _get_array_number(4, b);


    Tensor<double> t1(std::vector<uint64_t>({2,2}));
    Tensor<double> t2(std::vector<uint64_t>({2,2}));

    std::vector<std::vector<double>> lv1{{1,2},{3,4}};
    std::vector<std::vector<double>> lv2{{10,20},{30,40}};
    init_2d_tensor(t1, lv1);
    init_2d_tensor(t2, lv2);


    std::vector<Tensor<double>> tensors{t1, t2};
    auto d = concat_along_axis(tensors, 1);

    std::cout << d.ToString() << std::endl;
    
    // std::cout << t1.At({1,1})<< std::endl;
    // std::cout << t2.At({1,1})<< std::endl;

    // auto t3 = ConcatenateTensors(std::vector<Tensor<double>>({t1, t2}));

    // auto blub = t3.shape();

    // std::cout << blub.size() << std::endl;

    // print_3d_tensor(t3);

    int a = 1+1;
}