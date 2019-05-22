//          Copyright Maarten L. Hekkelman, 2019
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cassert>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <experimental/type_traits>

#include <zeep/el/element_fwd.hpp>
#include <zeep/el/traits.hpp>
#include <zeep/el/factory.hpp>
#include <zeep/el/to_element.hpp>
#include <zeep/el/from_element.hpp>

namespace zeep
{
namespace el
{

template<typename,typename = void>
struct element_serializer
{
	template<typename T>
	static auto to_element(element& j, T&& v)
		noexcept(noexcept(::zeep::el::detail::to_element(j, std::forward<T>(v))))
		-> decltype(::zeep::el::detail::to_element(j, std::forward<T>(v)))
	{
		::zeep::el::detail::to_element(j, std::forward<T>(v));
	}

	template<typename T>
	static auto from_element(const element& j, T& v)
		noexcept(noexcept(::zeep::el::detail::from_element(j, v)))
		-> decltype(::zeep::el::detail::from_element(j, v))
	{
		::zeep::el::detail::from_element(j, v);
	}

};

}
}