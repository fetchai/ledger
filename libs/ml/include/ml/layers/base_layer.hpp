#include "math/ndarray.hpp"
#include "vectorise/memory/array.hpp"
namespace fetch {
namespace ml {
namespace layers {

template<typename T, typename C = memory::SharedArray<T>>
class BaseLayer : public math::NDArray<T, C> {

 public:

  using data_type = T;
  BaseLayer &operator=(BaseLayer const &other) = default;
  void SetLayerSize(std::size_t layer_size) {layer_size_ = layer_size;}
  std::size_t LayerSize() { return layer_size_; }
  void SetWeightsMatrix(math::NDArray<T, C> new_weights_matrix) {weights_matrix_ = new_weights_matrix;}
  math::NDArray<T, C> WeightsMatrix() {return weights_matrix_;}


 protected:
  std::size_t layer_size_;
  math::NDArray<T, C> weights_matrix_;
 private:

};

} // layers
} // ml
} //fetch