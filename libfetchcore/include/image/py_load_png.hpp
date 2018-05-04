#ifndef LIBFETCHCORE_IMAGE_LOAD_PNG_HPP
#define LIBFETCHCORE_IMAGE_LOAD_PNG_HPP
#include "image/load_png.hpp"

#include"fetch_pybind.hpp"

namespace fetch
{
namespace image
{

void BuildFileReadErrorException(pybind11::module &module) {
  namespace py = pybind11;
  py::class_<FileReadErrorException, std::exception>(module, "FileReadErrorException" )
    .def(py::init< const std::string &, const std::string & >())
    .def("what", &FileReadErrorException::what);

}
};
};

#endif
