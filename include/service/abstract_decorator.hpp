#ifndef SERVICE_ABSTRACT_DECORATOR_HPP
#define SERVICE_ABSTRACT_DECORATOR_HPP
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
#endif
