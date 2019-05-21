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

namespace zeep
{
namespace detail
{

template<typename T, std::enable_if_t<std::is_same<T, bool>::value, int> = 0>
void to_element(element& v, T b)
{
	factory<value_type::boolean>::construct(v, b);
}

template<typename T, std::enable_if_t<std::is_constructible<T, std::string>::value, int> = 0>
void to_element(element& v, const T& s)
{
	factory<value_type::string>::construct(v, s);
}

inline void to_element(element& v, std::string&& s)
{
	factory<value_type::string>::construct(v, std::move(s));
}

template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
void to_element(element& v, T f)
{
	factory<value_type::number_float>::construct(v, f);
}

template<typename T, std::enable_if_t<std::is_integral<T>::value and std::is_signed<T>::value and not std::is_same<T, bool>::value, int> = 0>
void to_element(element& v, T i)
{
	factory<value_type::number_int>::construct(v, i);
}

template<typename T, std::enable_if_t<std::is_integral<T>::value and std::is_unsigned<T>::value and not std::is_same<T, bool>::value, int> = 0>
void to_element(element& v, T u)
{
	factory<value_type::number_uint>::construct(v, u);
}

template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
void to_element(element& v, T e)
{
	using int_type = typename std::underlying_type<T>::type;
	factory<value_type::number_int>::construct(v, static_cast<int_type>(e));
}

inline void to_element(element& j, const std::vector<bool>& v)
{
	factory<value_type::array>::construct(j, v);
}

template<typename T, std::enable_if_t<is_array_type<T>::value, int> = 0>
void to_element(element& j, const T& arr)
{
	factory<value_type::array>::construct(j, arr);
}

template<typename T, std::enable_if_t<std::is_convertible<element,T>::value, int> = 0>
void to_element(element& j, const std::valarray<T>& arr)
{
	factory<value_type::array>::construct(j, std::move(arr));
}

template<typename J>
void to_element(element& j, const typename J::array_type& arr)
{
	factory<value_type::array>::construct(j, std::move(arr));
}

template<typename J, typename T, size_t N,
	std::enable_if_t<not std::is_constructible<typename J::string_type, const T(&)[N]>::value, int> = 0>
void to_element(J& j, const T(&arr)[N])
{
	factory<value_type::array>::construct(j, std::move(arr));
} 

template<typename T, std::enable_if_t<is_object_type<element,T>::value, int> = 0>
void to_element(element& j, const T& obj)
{
	factory<value_type::object>::construct(j, std::move(obj));
}

template<typename J>
void to_element(J& j, const J& obj)
{
	factory<value_type::object>::construct(j, std::move(obj));
}

struct to_element_fn
{
    template<typename T>
    auto operator()(element& j, T&& val) const noexcept(noexcept(to_element(j, std::forward<T>(val))))
    -> decltype(to_element(j, std::forward<T>(val)), void())
    {
        return to_element(j, std::forward<T>(val));
    }
};

namespace
{
    constexpr const auto& to_element = typename ::zeep::detail::to_element_fn{};
}

}
}
