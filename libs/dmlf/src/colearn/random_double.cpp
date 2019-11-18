#include "dmlf/colearn/random_double.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

   RandomDouble::RandomDouble()
     : cache_(0)
     , rd_()
     , twister_(rd_())
     , underlying_(0.0, 1.0)
  {
    GetNew();
  }

  double RandomDouble::GetAgain() const
  {
    return cache_;
  }
  double RandomDouble::GetNew()
  {
    cache_ = underlying_(twister_);
    return cache_;
  }
  void RandomDouble::Seed(Bytes const &/*data*/)
  {
    // Do nothing at the moment.
  }
  void RandomDouble::Seed(CBytes const &/*data*/)
  {
    // Do nothing at the moment.
  }

  void RandomDouble::Set(double forced_value)
  {
     cache_ = forced_value;
  }
}
}
}
