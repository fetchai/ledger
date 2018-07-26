#pragma once
namespace fetch {
namespace service {

class AbstractDecorator {
 public:
  virtual void Apply(serializer_type &serializer,
                     byte_array::ConstByteArray const &data) = 0;
  virtual void Unapply(serializer_type &serializer,
                       byte_array::ConstByteArray const &data) = 0;
};
}
}
