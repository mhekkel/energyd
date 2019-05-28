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

template<typename T>
class name_value_pair
{
  public:
	name_value_pair(const char* name, T& value)
		: m_name(name), m_value(value) {}
	name_value_pair(const name_value_pair& other)
		: m_name(other.m_name), m_value(const_cast<T&>(other.m_value)) {}

	const char* name() const		{ return m_name; }
	T&			value() 			{ return m_value; }
	const T&	value() const		{ return m_value; }

  private:
	const char* m_name;
	T&			m_value;
};

template<typename T>
class element_nvp : public name_value_pair<T>
{
  public:
	element_nvp(const char* name, T& value) : name_value_pair<T>(name, value) {}
	element_nvp(const element_nvp& other) : name_value_pair<T>(other) {}
};

template<typename T>
class attribute_nvp : public name_value_pair<T>
{
  public:
	attribute_nvp(const char* name, T& value) : name_value_pair<T>(name, value) {}
	attribute_nvp(const attribute_nvp& other) : name_value_pair<T>(other) {}
};

struct serializer
{
	serializer() {}

	template<typename T>
	serializer& operator&(const element_nvp<T>& nvp)
	{
		serialize_element(nvp.name(), nvp.value());
		return *this;
	}

	template<typename T>
	serializer& operator&(const attribute_nvp<T>& nvp)
	{
		serialize_attribute(nvp.name(), nvp.value());
		return *this;
	}

	template<typename T>
	void serialize_element(const char* name, const T& data);

	template<typename T>
	void serialize_attribute(const char* name, const T& data);

	::zeep::el::element	m_root;
};

template<typename T, typename = void>
struct value_serializer {};

template<>
struct value_serializer<bool>
{
	std::string serialize_value(bool b)
	{
		return b ? "true" : "false";
	}

	bool deserialize_value(const std::string& value)
	{
		return value == "true";
	}
};

template<typename T>
struct value_serializer<T, std::enable_if<std::is_integral<T>::value and not std::is_same<T,bool>::value>>
{
	std::string serialize_value(int64_t v)
	{
		return std::to_string(v);
	}

	int64_t deserialize_value(const std::string& value)
	{
		return std::stoll(value);
	}
};

template<>
struct value_serializer<std::string>
{
	std::string serialize_value(const std::string& v)
	{
		return v;
	}

	std::string deserialize_value(const std::string& v)
	{
		return v;
	}
};

template<typename A, typename T>
using ampersand_operator_t = decltype(std::declval<A&>() & std::declval<T&>());

template<typename T, typename Archive>
using serialize_function = decltype(std::declval<T&>().serialize(std::declval<Archive&>(), std::declval<unsigned long>()));

template<typename T, typename Archive, typename = void>
struct has_serialize : std::false_type {};

template<typename T, typename Archive>
struct has_serialize<T, Archive,
	typename std::enable_if<std::is_class<T>::value and std::experimental::is_detected_v<ampersand_operator_t,Archive,T>>::type>
{
	static constexpr bool value = std::experimental::is_detected_v<serialize_function,T,Archive>;
};

template<typename T>
struct value_serializer<T, std::enable_if<has_serialize<T,serializer>::value>>
{
	std::string serialize_value(const T& v)
	{
		serializer sr;
		v.serialize(sr, 0);

	}

	std::string deserialize_value(const std::string& v)
	{
		return v;
	}

};





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