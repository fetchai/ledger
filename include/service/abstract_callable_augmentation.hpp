#ifndef SERVICE_ABSTRACT_CALLABLE_AUGMENTATION_HPP
#define SERVICE_ABSTRACT_CALLABLE_AUGMENTATION_HPP
namespace fetch {
namespace service {

class AbstractCallableAugmentation
{
public:
  template<typename ...Args>
  virtual operator()(Args ...args) = 0;  
};
  
};
};
#endif
