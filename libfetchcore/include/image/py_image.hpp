#ifndef LIBFETCHCORE_IMAGE_IMAGE_HPP
#define LIBFETCHCORE_IMAGE_IMAGE_HPP
#include "image/image.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
namespace fetch
{
namespace image
{
namespace colors
{

template< typename V, std::size_t B, std::size_t C >
void BuildAbstractColor(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<AbstractColor< V, B, C >>(module, custom_name )
    .def(py::init< const typename fetch::image::colors::AbstractColor<V, B, C>::container_type & >())
    .def("operator[]", &AbstractColor< V, B, C >::operator[]);

}
};
};
};
namespace fetch
{
namespace image
{

template< typename T >
void BuildImageType(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<ImageType< T >, fetch::math::linalg::Matrix<typename T::container_type>>(module, custom_name )
    .def(py::init<  >())
    .def(py::init< ImageType<T> && >())
    .def(py::init< const ImageType<T> & >())
    .def(py::init< const typename fetch::image::ImageType< T >::super_type & >())
    .def(py::init< typename fetch::image::ImageType< T >::super_type && >())
    .def(py::init< const std::size_t &, const std::size_t & >())
    .def("Load", &ImageType< T >::Load);
    //    .def(py::self = py::self );

}
};
};

#endif
