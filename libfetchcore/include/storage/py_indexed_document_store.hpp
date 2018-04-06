#ifndef LIBFETCHCORE_STORAGE_INDEXED_DOCUMENT_STORE_HPP
#define LIBFETCHCORE_STORAGE_INDEXED_DOCUMENT_STORE_HPP
//#include "storage/indexed_document_store.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace storage
{

template< typename B >
void BuildIndexedDocumentStore(std::string const &custom_name, pybind11::module &module) {
  /*
  namespace py = pybind11;
  py::class_<IndexedDocumentStore< B >>(module, custom_name )
    .def(py::init<>()) 
    .def("Load", &IndexedDocumentStore< B >::Load)
    .def("New", &IndexedDocumentStore< B >::New)
    .def("Clear", &IndexedDocumentStore< B >::Clear)
    .def("Delete", &IndexedDocumentStore< B >::Delete);
  */

}
};
};

#endif
