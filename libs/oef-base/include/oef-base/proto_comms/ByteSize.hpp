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

#include <type_traits>
#include <utility>

namespace fetch {

namespace byte_size {

class ByteSize {
private:
	struct Yes {};
	struct No {};
	
	template<class T> static constexpr auto HasByteSizeLong(T &&t) -> decltype(t.ByteSizeLong(), Yes{});
	static constexpr No HasByteSizeLong(...);

	template<class T> static constexpr bool has_long_v = std::is_same<decltype(HasByteSizeLong(std::declval<T>())), Yes>::value;
public:
	template<class T> static constexpr std::enable_if_t<has_long_v<T>, std::size_t> Call(T &&t) {
		return t.ByteSizeLong();
	}

	template<class T> static constexpr std::enable_if_t<!has_long_v<T>, std::size_t> Call(T &&t) {
		return t.ByteSize();
	}
};

}

}
