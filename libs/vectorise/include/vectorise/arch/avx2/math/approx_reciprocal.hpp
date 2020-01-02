#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

namespace fetch {
namespace vectorise {

inline VectorRegister<float, 128> approx_reciprocal(VectorRegister<float, 128> const &x)
{

  return VectorRegister<float, 128>(_mm_rcp_ps(x.data()));
}

inline VectorRegister<float, 256> approx_reciprocal(VectorRegister<float, 256> const &x)
{

  return VectorRegister<float, 256>(_mm256_rcp_ps(x.data()));
}

inline VectorRegister<double, 128> approx_reciprocal(VectorRegister<double, 128> const &x)
{
  // TODO(issue 3): Test this function
  return VectorRegister<double, 128>(_mm_cvtps_pd(_mm_rcp_ps(_mm_cvtpd_ps(x.data()))));
}

inline VectorRegister<double, 256> approx_reciprocal(VectorRegister<double, 256> const &x)
{
  // TODO(issue 3): Test this function
  return VectorRegister<double, 256>(_mm256_cvtps_pd(_mm256_rcp_ps(_mm256_cvtpd_ps(x.data()))));
}

}  // namespace vectorise
}  // namespace fetch
