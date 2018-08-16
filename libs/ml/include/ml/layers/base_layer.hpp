#include "math/ndarray.hpp"
#include "vectorise/memory/array.hpp"
namespace fetch {
namespace ml {
namespace layers {

template <typename DATA_TYPE>
class BaseLayer
{

  using container_type = memory::SharedArray<DATA_TYPE>;
  using array_type     = math::NDArray<DATA_TYPE, container_type>;

public:
  BaseLayer() {}
  BaseLayer(std::size_t layer_size) { layer_size_ = layer_size; }

  BaseLayer &operator=(BaseLayer const &other) = default;

  void        SetLayerSize(std::size_t layer_size) { layer_size_ = layer_size; }
  std::size_t LayerSize() { return layer_size_; }
  void SetWeightsMatrix(array_type new_weights_matrix) { weights_matrix_ = new_weights_matrix; }
  array_type WeightsMatrix() { return weights_matrix_; }

protected:
  std::size_t layer_size_;
  array_type  weights_matrix_;

private:
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch