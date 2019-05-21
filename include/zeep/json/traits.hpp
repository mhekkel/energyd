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

namespace zeep
{

namespace detail
{

template<typename> struct is_element : std::false_type {};
template<> struct is_element<element> : std::true_type {};

template <typename T>
using mapped_type_t = typename T::mapped_type;

template <typename T>
using key_type_t = typename T::key_type;

template <typename T>
using value_type_t = typename T::value_type;

template <typename T>
using iterator_t = typename T::iterator;

template <typename T, typename... Args>
using to_element_function = decltype(T::to_element(std::declval<Args>()...));

template <typename T, typename... Args>
using from_element_function = decltype(T::from_element(std::declval<Args>()...));

template<typename T, typename = void>
struct has_to_element : std::false_type {};

template<typename T>
struct has_to_element<T, std::enable_if_t<not is_element<T>::value>>
{
	using serializer = element_serializer<T, void>;
	static constexpr bool value =
		std::experimental::is_detected_exact<void, to_element_function, serializer, element&, T>::value;
};

template<typename T, typename = void>
struct has_from_element : std::false_type {};

template<typename T>
struct has_from_element<T, std::enable_if_t<not is_element<T>::value>>
{
	using serializer = element_serializer<T, void>;
	static constexpr bool value =
		std::experimental::is_detected_exact<void, from_element_function, serializer, const element&, T&>::value;
};

template<typename T, typename = void>
struct is_complete_type : std::false_type {};

template<typename T>
struct is_complete_type<T, decltype(void(sizeof(T)))> : std::true_type {};

template<typename T, typename = void>
struct is_array_type : std::false_type {};

template<typename T>
struct is_array_type<T,
	std::enable_if_t<
		std::experimental::is_detected<value_type_t, T>::value and
		std::experimental::is_detected<iterator_t, T>::value>>
{
    static constexpr bool value =
        std::is_constructible<element, typename T::value_type>::value;
};

template<typename J, typename T, typename = void>
struct is_object_type : std::false_type {};

template<typename J, typename T>
struct is_object_type<J, T, std::enable_if_t<
	std::experimental::is_detected<mapped_type_t, T>::value and
	std::experimental::is_detected<key_type_t, T>::value>>
{
	using map_t = typename J::object_type;
	static constexpr bool value = 
		std::is_constructible<std::string,typename map_t::key_type>::value and
		std::is_constructible<element,typename map_t::mapped_type>::value; 
};

template<typename T, typename = void>
struct is_compatible_type_impl : std::false_type {};

template<typename T>
struct is_compatible_type_impl<T, std::enable_if_t<is_complete_type<T>::value>>
{
	static constexpr bool value = has_to_element<T>::value;
};

template<typename T>
struct is_compatible_type : is_compatible_type_impl<T> {};

}
}
