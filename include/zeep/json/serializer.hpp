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

#include <zeep/json/element_fwd.hpp>
#include <zeep/json/traits.hpp>
#include <zeep/json/factory.hpp>
#include <zeep/json/to_element.hpp>
#include <zeep/json/from_element.hpp>

namespace zeep
{

template<typename,typename = void>
struct element_serializer
{
	template<typename T>
	static auto to_element(element& j, T&& v)
		noexcept(noexcept(::zeep::detail::to_element(j, std::forward<T>(v))))
		-> decltype(::zeep::detail::to_element(j, std::forward<T>(v)))
	{
		::zeep::detail::to_element(j, std::forward<T>(v));
	}

	template<typename T>
	static auto from_element(const element& j, T& v)
		noexcept(noexcept(::zeep::detail::from_element(j, v)))
		-> decltype(::zeep::detail::from_element(j, v))
	{
		::zeep::detail::from_element(j, v);
	}

};

}