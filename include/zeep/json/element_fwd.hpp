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

namespace zeep
{

class element;

namespace detail
{

enum class value_type : std::uint8_t
{
	null,
	object,
	array,
	string,
	number_int,
	number_float,
	boolean
};

class element_reference;

}

template<typename,typename>
struct element_serializer;

}