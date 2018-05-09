#include <vector>
namespace fetch {
namespace serializers {

template <typename T>
void Serialize(T &serializer, std::vector<uint64_t> const &vec) {
  uint64_t bytes = vec.size() * sizeof(uint64_t);
  serializer.Allocate(sizeof(uint64_t) + bytes);
  uint64_t size = vec.size();
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(&size),
                        sizeof(uint64_t));
  serializer.WriteBytes(reinterpret_cast<uint8_t const *>(vec.data()), bytes);
}

template <typename T>
void Deserialize(T &serializer, std::vector<uint64_t> &vec) {
  uint64_t size;
  serializer.ReadBytes(reinterpret_cast<uint8_t *>(&size), sizeof(uint64_t));
  uint64_t bytes = size * sizeof(uint64_t);
  vec.resize(size);
  serializer.WriteBytes(reinterpret_cast<uint8_t *>(vec.data()), bytes);
}
};
};
#include "random/lfg.hpp"
#include "serializers/byte_array_buffer.hpp"
#include "storage/file_object.hpp"

#include "unittest.hpp"

#include <iostream>
#include <stack>
using namespace fetch::storage;
using namespace fetch::serializers;

#define TYPE uint64_t

int main() {
  SCENARIO("we use the implementation to write an index file") {
    typedef typename FileObjectImplementation<>::stack_type stack_type;
    stack_type stack;
    stack.New("variant_stack_fb_1.db");
    FileObjectImplementation fbo(0, stack);

    std::vector<uint64_t> positions;
    ByteArrayBuffer buffer;
  };

  return 0;
}
