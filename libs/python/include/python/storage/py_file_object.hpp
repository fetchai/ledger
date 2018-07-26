#pragma once
//#include "storage/file_object.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace storage {

template <typename S>
void BuildFileObjectImplementation(std::string const &custom_name,
                                   pybind11::module & module)
{
  /*
  namespace py = pybind11;
  py::class_<FileObjectImplementation< S >>(module, custom_name )
    .def(py::init< const uint64_t &,
  fetch::storage::FileObjectImplementation::stack_type & >()) .def("Read",
  &FileObjectImplementation< S >::Read) .def("Tell", &FileObjectImplementation<
  S >::Tell) .def("Write", &FileObjectImplementation< S >::Write)
    .def("file_position", &FileObjectImplementation< S >::file_position)
    .def("Seek", &FileObjectImplementation< S >::Seek)
    .def("Shrink", &FileObjectImplementation< S >::Shrink)
    .def("Size", &FileObjectImplementation< S >::Size);
  */
}
};  // namespace storage
};  // namespace fetch
