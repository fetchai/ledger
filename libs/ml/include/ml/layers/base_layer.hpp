#include "math/ndarray.hpp"
#include "vectorise/memory/array.hpp"
namespace fetch {
namespace ml {
namespace layers {

template<typename T, typename C = memory::SharedArray<T>>
virtual
class BaseLayer : public NDArray<T, C> {
 public:
  std::size_t LayerSize() { return layer_size_; }

 private:
  std::size_t layer_size_;
}

}; // layers
}; // ml
}; //fetch