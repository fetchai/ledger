#include "python/fetch_pybind.hpp"
#include <pybind11/embed.h>
namespace py = pybind11;

#include "python/network/swarm/py_swarm_agent_api.hpp"
#include "network/swarm/swarm_agent_api.hpp"

#include "network/swarm/swarm_node.hpp"
#include "network/swarm/swarm_http_interface.hpp"
#include "network/swarm/swarm_service.hpp"
#include "network/swarm/swarm_peer_location.hpp"
#include "network/swarm/swarm_random.hpp"
#include "swarm_agent_naive.hpp"

#include <iostream>
#include <unistd.h>
#include <string>
#include <time.h>

PYBIND11_EMBEDDED_MODULE(fetchnetwork, module) {
  namespace py = pybind11;

  py::module ns_fetch_network_swarm = module.def_submodule("swarm");
  fetch::swarm::BuildSwarmAgentApi(ns_fetch_network_swarm);
}


class PythonContext
{
public:
  typedef std::shared_ptr<fetch::swarm::PySwarm> SWARM_P;
  typedef std::unique_ptr<py::scoped_interpreter> INTERP_P;
  typedef py::dict LOCALS;
  typedef std::shared_ptr<LOCALS> LOCALS_P;

  LOCALS_P locals;
  SWARM_P pySwarm;
  INTERP_P interpreter;

  PythonContext()
  {
  }

  virtual ~PythonContext()
  {
    if (pySwarm)
      {
        pySwarm -> Stop();
      }
    locals.reset();
    interpreter.reset();
  }

  void begin()
  {
    interpreter = INTERP_P(new py::scoped_interpreter());

    locals = std::make_shared<py::dict>();
  }

  void done()
  {
    try
      {
        pySwarm = (*locals)["swarm"].cast<SWARM_P>();
        pySwarm -> Start();
      }
    catch (std::exception &x)
      {
      }
    pybind11::gil_scoped_release release;
    sleep(10);
  }

  void runStr(const string &text)
  {
    cerr << "BAD"<<endl;
    //execute(&(py::exec), text);
    done();
  }

    /* See -- 
https://github.com/pybind/pybind11/issues/1296
https://github.com/cython/cython/issues/1877

    */
  void runFile(const string &fn)
  {
    begin();
    //py::eval_file(fn, py::globals(), *locals);
    py::eval_file(fn, py::globals());
    done();
  }

  void runStdin()
  {
    cerr << "BAD"<<endl;
    std::string program;
    std::string input_line;
    while(std::cin) {
      getline(std::cin, input_line);
      program += "\n" + input_line;
    };
    runStr(program);
  }


  //py::object scope = py::module::import("__main__").attr("__dict__");


};

int main(int argc, char *argv[])
{
  PythonContext context;
  if (argc > 1)
    {
      if (std::string(argv[1]) == "test")
        {
          context.runStr(R"(
    import fetchnetwork.swarm
    swarm = fetchnetwork.swarm.Swarm(0, 9000, 3, 1000, 10000)

    def idleHandler():
        print('IDLE')

    swarm.OnIdle(idleHandler)

)");
        }
      else
        {
          context.runFile(argv[1]);
        }
    }
  else
    {
      context.runStdin();
    }
  cout << "bye"<< endl;
}
