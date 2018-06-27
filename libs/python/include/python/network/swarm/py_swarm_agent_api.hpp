#ifndef LIBFETCHNETWORK_SWARM_SWARM_AGENT_API
#define LIBFETCHNETWORK_SWARM_SWARM_AGENT_API

#include "network/swarm/swarm_peer_location.hpp"
#include "py_swarm.hpp"
#include "python/fetch_pybind.hpp"
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>

namespace fetch
{
namespace swarm
{

void BuildSwarmAgentApi(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<fetch::swarm::PySwarm, std::shared_ptr<fetch::swarm::PySwarm>>(module, "Swarm" )
    .def(py::init<unsigned int, uint16_t, unsigned int, unsigned int, unsigned int>())
    .def("Start", &PySwarm::Start)
    .def("Stop", &PySwarm::Stop)
    .def("OnPeerless", &PySwarm::OnPeerless)

    .def("OnIdle", &PySwarm::OnIdle)
    .def("OnPingSucceeded", &PySwarm::OnPingSucceeded)
    .def("OnPingFailed", &PySwarm::OnPingFailed)
    .def("OnNewPeerDiscovered", &PySwarm::OnNewPeerDiscovered)
    .def("OnPeerDiscoverFail", &PySwarm::OnPeerDiscoverFail)
    .def("DoPing", &PySwarm::DoPing)

    // We *have* an entry point for this but currently don't use it because we treat Ping as DiscoverPeers
    //.def("DoDiscoverPeers", &PySwarm::DoDiscoverPeers)
    .def("OnNewPeerDiscovered", &PySwarm::OnNewPeerDiscovered)
    .def("OnPeerDiscoverFail", &PySwarm::OnPeerDiscoverFail)

    .def("DoBlockSolved", &PySwarm::DoBlockSolved)
    .def("DoDiscoverBlocks", &PySwarm::DoDiscoverBlocks)
    .def("OnNewBlockIdFound", &PySwarm::OnNewBlockIdFound)
    .def("OnBlockIdRepeated", &PySwarm::OnBlockIdRepeated)
    .def("DoGetBlock", &PySwarm::DoGetBlock)
    .def("OnNewBlockAvailable", &PySwarm::OnNewBlockAvailable)
    .def("GetBlock", &PySwarm::GetBlock)
    .def("VerifyBlock", &PySwarm::VerifyBlock)

    // TODO(katie) implement the back end of these.
    //.def("DoTransactionListBuilt", &PySwarm::DoTransactionListBuilt)
    //.def("OnNewTxnListIdFound", &PySwarm::OnNewTxnListIdFound)
    //.def("DoGetTxnList", &PySwarm::DoGetTxnList)
    //.def("OnNewTxnListAvailable", &PySwarm::OnNewTxnListAvailable)
    //.def("GetTxnList", &PySwarm::GetTxnList)

    .def("AddKarma", &PySwarm::AddKarma)
    .def("AddKarmaMax", &PySwarm::AddKarmaMax)
    .def("GetKarma", &PySwarm::GetKarma)
    .def("GetCost", &PySwarm::GetCost)
    .def("GetPeers", &PySwarm::GetPeers)
    .def("queryOwnLocation", &PySwarm::queryOwnLocation)
    ;
}

};
};

#endif
