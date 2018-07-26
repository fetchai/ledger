#ifndef LIBFETCHCORE_CHAIN_CONSENSUS_PROOF_OF_WORK_HPP
#define LIBFETCHCORE_CHAIN_CONSENSUS_PROOF_OF_WORK_HPP
#include "chain/consensus/proof_of_work.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace chain {
namespace consensus {

void BuildProofOfWork(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ProofOfWork, math::BigUnsigned>(module, "ProofOfWork")
      .def(py::init<>())
      .def(py::init<fetch::chain::consensus::ProofOfWork::header_type>())
      .def("target", &ProofOfWork::target)
      .def("SetTarget", &ProofOfWork::SetTarget)
      .def("operator()", &ProofOfWork::operator())
      .def("header", &ProofOfWork::header)
      .def("SetHeader", &ProofOfWork::SetHeader)
      .def("digest", &ProofOfWork::digest);
}
};  // namespace consensus
};  // namespace chain
};  // namespace fetch

#endif
