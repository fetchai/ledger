#include "dmlf/vm_wrapper_python3.hpp"

//#include <python.h>

namespace fetch {
namespace dmlf {

  typedef int PyEnvName;

  class PyEnvInner
  {
  public:
    static int py_env_counter;

    PyEnvInner()
    {
      if (py_env_counter == 0)
      {
        //::Py_Initialize();
      }
      py_env_counter++;
    }

    virtual ~PyEnvInner()
    {
      py_env_counter--;
      if (py_env_counter == 0)
      {
        //::Py_FinalizeEx();
      }
    }
    
  };

  int PyEnvInner::py_env_counter = 0;

  VmWrapperPython3::VmWrapperPython3()
  {
    py_ = std::make_shared<PyEnvInner>();
  }
  VmWrapperPython3::~VmWrapperPython3()
  {
    py_ . reset();
  }
}
}
