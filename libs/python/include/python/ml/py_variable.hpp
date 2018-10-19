#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "ml/ops/ops.hpp"
#include "ml/session.hpp"
#include "python/fetch_pybind.hpp"
#include <pybind11/stl.h>

namespace fetch {
namespace ml {

template <typename ArrayType>
void BuildVariable(std::string const &custom_name, pybind11::module &module)
{

  using SelfType    = fetch::ml::Variable<ArrayType>;
  using SelfPtrType = std::shared_ptr<SelfType>;
  using SessionType = fetch::ml::SessionManager<ArrayType, SelfType>;

  namespace py = pybind11;
  py::class_<SelfType, std::shared_ptr<SelfType>>(module, custom_name.c_str())
      .def(py::init<>())

      //      .def(py::init<ArrayType const &>())
      //      .def(py::init<SessionType &, ArrayType const &>())
      .def("Dot", [](SelfPtrType a, SelfPtrType b,
                     SessionType &sess) { return fetch::ml::ops::Dot(a, b, sess); })
      .def("Relu", [](SelfPtrType a, SessionType &sess) { return fetch::ml::ops::Relu(a, sess); })
      .def("ReduceSum", [](SelfPtrType a, std::size_t axis,
                           SessionType &sess) { return fetch::ml::ops::ReduceSum(a, axis, sess); })
      .def("size", [](SelfType &a) { return a.size(); })
      .def("shape", [](SelfType &a) { return a.shape(); })
      .def("Reshape", [](SelfType &a, std::size_t i, std::size_t j) { a.Reshape(i, j); })
      .def("data", [](SelfType &a) { return a.data(); })
      .def("SetData", [](SelfType &s, ArrayType const &v) { s.SetData(v); })
      .def("SetRange", [](SelfType &ret, std::vector<std::vector<std::size_t>> &range, SelfType const &s)
      {
        ret.data().SetRange(range, s.data());
      })

      .def("Grads", [](SelfType &s) { return s.grad(); })
      .def("FromNumpy",
           [](SelfType &s, py::array_t<typename ArrayType::Type> arr) {
             auto buf        = arr.request();
             using size_type = typename ArrayType::size_type;
             if (buf.ndim != 2)
             {
               throw std::runtime_error("Dimension must be exactly two.");
             }

             typename ArrayType::Type *ptr = (typename ArrayType::Type *)buf.ptr;
             std::size_t               idx = 0;
             s.Reshape(size_type(buf.shape[0]), size_type(buf.shape[1]));
             for (std::size_t i = 0; i < std::size_t(buf.shape[0]); ++i)
             {
               for (std::size_t j = 0; j < std::size_t(buf.shape[1]); ++j)
               {
                 s[idx] = ptr[idx];
                 ++idx;
               }
             }
           })
      .def("ToNumpy",
           [](SelfType &s) {
             auto result =
                 py::array_t<typename ArrayType::Type>({s.data().shape()[0], s.data().shape()[1]});
             auto buf = result.request();

             typename ArrayType::Type *ptr = (typename ArrayType::Type *)buf.ptr;
             for (size_t i = 0; i < s.size(); ++i)
             {
               ptr[i] = s[i];
             }
             return result;
           })
      .def("Get",
           [](const SelfType &s, std::size_t i) {
             if (i >= s.size())
             {
               throw py::index_error();
             }
             return s[i];
             //             return s.data().Get(i);
           })
      .def("Set",
           [](SelfType &s, std::size_t i, typename ArrayType::Type v) {
             if (i >= s.size())
             {
               throw py::index_error();
             }
             s[i] = v;
             //             s.data().Set(i, v);
           })
      .def("Set",
           [](SelfType &s, std::size_t i, std::size_t j, typename ArrayType::Type v) {
             if (i >= s.size())
             {
               throw py::index_error();
             }
             s.At(i, j) = v;
             //             s.data().Set(i, v);
           })
      .def("__getitem__",
           [](const SelfType &s, std::size_t i) {
             if (i >= s.size())
             {
               throw py::index_error();
             }
             //             return s.data().Get(i);
             return s[i];
           })
      .def("__getitem__",
           [](SelfType &s, py::tuple index) {
             if (py::len(index) != 2)
             {
               throw py::index_error();
             }
             int a = index[0].cast<int>();
             int b = index[1].cast<int>();
             if (a < 0)
             {
               a += int(s.shape()[0]);
             }
             if (b < 0)
             {
               b += int(s.shape()[1]);
             }
             std::size_t i = std::size_t(a);
             std::size_t j = std::size_t(b);

             if (i >= s.shape()[0])
             {
               throw py::index_error();
             }
             if (j >= s.shape()[1])
             {
               throw py::index_error();
             }

             return s.At(i, j);
           })
      .def("__setitem__",
           [](SelfType &s, std::size_t i, typename ArrayType::Type v) {
             if (i >= s.size())
             {
               throw py::index_error();
             }
             s.data().Set(i, v);
           })
      .def("__setitem__", [](SelfType &s, py::tuple index, typename ArrayType::Type const &v) {
        if (py::len(index) != 2)
        {
          throw py::index_error();
        }
        int a = index[0].cast<int>();
        int b = index[1].cast<int>();
        if (a < 0)
        {
          a += int(s.shape()[0]);
        }
        if (b < 0)
        {
          b += int(s.shape()[1]);
        }
        std::size_t i = std::size_t(a);
        std::size_t j = std::size_t(b);

        if (std::size_t(i) >= s.shape()[0])
        {
          throw py::index_error();
        }
        if (std::size_t(j) >= s.shape()[1])
        {
          throw py::index_error();
        }
        s.Set(i, j, v);
      });
}

}  // namespace ml
}  // namespace fetch
