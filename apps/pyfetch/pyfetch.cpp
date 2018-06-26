#include <iostream>
#include <unistd.h>
#include <string>
#include <time.h>



#include "network/parcels/swarm_agent_naive.hpp"
#include "network/swarm/swarm_agent_api.hpp"
#include "network/swarm/swarm_http_interface.hpp"
#include "network/swarm/swarm_node.hpp"
#include "network/swarm/swarm_peer_location.hpp"
#include "network/swarm/swarm_random.hpp"
#include "network/swarm/swarm_service.hpp"
#include "python/fetch_pybind.hpp"
#include "python/network/swarm/py_swarm_agent_api.hpp"
#include <pybind11/embed.h>

PYBIND11_EMBEDDED_MODULE(fetchnetwork, module) {
  namespace py = pybind11;

  pybind11::module ns_fetch_network_swarm = module.def_submodule("swarm");
  fetch::swarm::BuildSwarmAgentApi(ns_fetch_network_swarm);
}


class PythonContext
{
public:
  typedef std::shared_ptr<fetch::swarm::PySwarm> SWARM_P;
  typedef std::unique_ptr<pybind11::scoped_interpreter> INTERP_P;
  typedef pybind11::dict LOCALS;
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
    interpreter = INTERP_P(new pybind11::scoped_interpreter());

    locals = std::make_shared<pybind11::dict>();
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

    /* See -- 
https://github.com/pybind/pybind11/issues/1296
https://github.com/cython/cython/issues/1877

    */
  void runFile(const string &fn)
  {
    begin();
    pybind11::eval_file(fn, pybind11::globals());
    done();
  }
};

int main(int argc, char *argv[])
{
  PythonContext context;
  if (argc > 1)
    {
      context.runFile(argv[1]);
    }
  else
    {
      std::cerr << "Please supply filenames to run" << std::endl;
      exit(1);
    }
}
