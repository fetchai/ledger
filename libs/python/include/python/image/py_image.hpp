#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "python/fetch_pybind.hpp"
#include "python/image/image.hpp"

namespace fetch {
namespace image {
namespace colors {

template <typename V, std::size_t B, std::size_t C>
void BuildAbstractColor(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<AbstractColor<V, B, C>>(module, custom_name.c_str())
      .def(
          py::init<const typename fetch::image::colors::AbstractColor<V, B, C>::container_type &>())
      .def("operator[]", &AbstractColor<V, B, C>::operator[]);
}
};  // namespace colors

template <typename T>
void BuildImageType(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<ImageType<T>, fetch::math::linalg::Matrix<typename T::container_type>>(
      module, custom_name.c_str())
      .def(py::init<>())
      //    .def(py::init< ImageType<T> && >())
      .def(py::init<const ImageType<T> &>())
      .def(py::init<const typename fetch::image::ImageType<T>::super_type &>())
      //    .def(py::init< typename fetch::image::ImageType< T >::super_type &&
      //    >())
      .def(py::init<const std::size_t &, const std::size_t &>())
      .def("Load", &ImageType<T>::Load);
  //    .def(py::self = py::self );
}
};  // namespace image
};  // namespace fetch
