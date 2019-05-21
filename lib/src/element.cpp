//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>

#include <zeep/json/element.hpp>

namespace zeep
{

/// empty factory with a certain type
element::element(value_type t)
	: m_type(t), m_data(t)
{
	validate();
}

/// default factory
element::element(std::nullptr_t)
	: element(value_type::null)
{
	validate();
}

element::element(const element& j)
	: m_type(j.m_type)
{
	j.validate();
	switch (m_type)
	{
		case value_type::null:			break;
		case value_type::array:			m_data = *j.m_data.m_array; break;
		case value_type::object:		m_data = *j.m_data.m_object; break;
		case value_type::string:		m_data = *j.m_data.m_string; break;
		case value_type::number_int:	m_data = j.m_data.m_int; break;
		case value_type::number_uint:	m_data = j.m_data.m_uint; break;
		case value_type::number_float:	m_data = j.m_data.m_float; break;
		case value_type::boolean:		m_data = j.m_data.m_boolean; break;
	}
	validate();
}

element::element(element&& j)
	: m_type(std::move(j.m_type)), m_data(std::move(j.m_data))
{
	j.validate();

	j.m_type = value_type::null;
	j.m_data = {};

	validate();
}

element::element(const detail::element_reference& r)
	: element(r.data())
{
	validate();
}

element::element(initializer_list_t init)
{
	bool isAnObject = std::all_of(init.begin(), init.end(), [](auto& ref)
		{ return ref->is_array() and ref->m_data.m_array->size() == 2 and ref->m_data.m_array->front().is_string(); });

	if (isAnObject)
	{
		m_type = value_type::object;
		m_data = value_type::object;

		for (auto& ref: init)
		{
			auto element = ref.data();
			m_data.m_object->emplace(
				std::move(*element.m_data.m_array->front().m_data.m_string),
				std::move(element.m_data.m_array->back())
			);
		}
	}
	else
	{
		m_type = value_type::array;
		m_data.m_array = create<array_type>(init.begin(), init.end());
	}
}

element::element(size_t cnt, const element& v)
	: m_type(value_type::array)
{
	m_data.m_array = create<array_type>(cnt, v);
	validate();
}

element& element::operator=(element j) noexcept(
		std::is_nothrow_move_constructible<value_type>::value and
		std::is_nothrow_move_assignable<value_type>::value and
		std::is_nothrow_move_constructible<element>::value and
		std::is_nothrow_move_assignable<element>::value)
{
	j.validate();

	using std::swap;

	swap(m_type, j.m_type);
	swap(m_data, j.m_data);

	validate();

	return *this; 
}

element::~element()
{
	validate();
	m_data.destroy(m_type);
}

std::string element::type_name() const
{
  switch (m_type)
	{
		case value_type::null:			return "null"; break;
		case value_type::array:			return "array"; break;
		case value_type::object:		return "object"; break;
		case value_type::string:		return "string"; break;
		case value_type::number_int:	return "number_int"; break;
		case value_type::number_uint:	return "number_uint"; break;
		case value_type::number_float:	return "number_float"; break;
		case value_type::boolean:		return "boolean"; break;
	}
}

// array access

element::reference element::at(size_t index)
{
	if (not is_array())
		throw std::runtime_error("Type should have been array to use at()");
	
	return m_data.m_array->at(index);
}

element::const_reference element::at(size_t index) const
{
	if (not is_array())
		throw std::runtime_error("Type should have been array to use at()");
	
	return m_data.m_array->at(index);
}

element::reference element::operator[](size_t index)
{
	if (is_null())
	{
		m_type = value_type::array;
		m_data.m_array = create<array_type>();
	}
	else if (not is_array())
		throw std::runtime_error("Type should have been array to use operator[]");
	
	if (index < m_data.m_array->size())
		m_data.m_array->resize(index + 1);
	
	return m_data.m_array->operator[](index);
}

element::const_reference element::operator[](size_t index) const
{
	if (not is_array())
		throw std::runtime_error("Type should have been array to use operator[]");
	
	return m_data.m_array->operator[](index);
}

// object member access

element::reference element::at(const typename object_type::key_type& key)
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use at()");
	
	return m_data.m_object->at(key);
}

element::const_reference element::at(const typename object_type::key_type& key) const
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use at()");
	
	return m_data.m_object->at(key);
}

element::reference element::operator[](const typename object_type::key_type& key)
{
	if (is_null())
	{
		m_type = value_type::object;
		m_data.m_object = create<object_type>();
	}
	else if (not is_object())
		throw std::runtime_error("Type should have been object to use operator[]");
	
	return m_data.m_object->operator[](key);
}

element::const_reference element::operator[](const typename object_type::key_type& key) const
{
	if (not is_object())
		throw std::runtime_error("Type should have been object to use operator[]");
	
	return m_data.m_object->operator[](key);
}

bool element::empty() const
{
	switch (m_type)
	{
		case value_type::null:
			return true;

	case value_type::array:
		return m_data.m_array->empty();
	
	case value_type::object:
		return m_data.m_object->empty();

	default:
		return false;
	}
}

size_t element::size() const
{
	switch (m_type)
	{
		case value_type::null:
			return 0;

	case value_type::array:
		return m_data.m_array->size();
	
	case value_type::object:
		return m_data.m_object->size();

	default:
		return 1;
	}
}

bool operator==(element::const_reference& lhs, element::const_reference& rhs)
{
	using detail::value_type;

	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::array:			return *lhs.m_data.m_array == *rhs.m_data.m_array;
			case value_type::object:		return *lhs.m_data.m_object == *rhs.m_data.m_object;
			case value_type::string:		return *lhs.m_data.m_string == *rhs.m_data.m_string;
			case value_type::number_int:	return lhs.m_data.m_int == rhs.m_data.m_int;
			case value_type::number_uint:	return lhs.m_data.m_uint == rhs.m_data.m_uint;
			case value_type::number_float:	return lhs.m_data.m_float == rhs.m_data.m_float;
			case value_type::boolean:		return lhs.m_data.m_boolean == rhs.m_data.m_boolean;
		}
	}
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_int)
		return lhs.m_data.m_float == static_cast<element::float_type>(rhs.m_data.m_int);
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_float)
		return static_cast<element::float_type>(lhs.m_data.m_int) == rhs.m_data.m_float;
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_uint)
		return lhs.m_data.m_float == static_cast<element::float_type>(rhs.m_data.m_uint);
	else if (lhs_type == value_type::number_uint and rhs_type == value_type::number_float)
		return static_cast<element::float_type>(lhs.m_data.m_uint) == rhs.m_data.m_float;
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_uint)
		return lhs.m_data.m_int == static_cast<element::int_type>(rhs.m_data.m_uint);
	else if (lhs_type == value_type::number_uint and rhs_type == value_type::number_int)
		return static_cast<element::int_type>(lhs.m_data.m_uint) == rhs.m_data.m_int;
	
	return false;
}

bool operator<(element::const_reference& lhs, element::const_reference& rhs)
{
	using detail::value_type;

	auto lhs_type = lhs.type();
	auto rhs_type = rhs.type();

	if (lhs_type == rhs_type)
	{
		switch (lhs_type)
		{
			case value_type::array:			return *lhs.m_data.m_array < *rhs.m_data.m_array;
			case value_type::object:		return *lhs.m_data.m_object < *rhs.m_data.m_object;
			case value_type::string:		return *lhs.m_data.m_string < *rhs.m_data.m_string;
			case value_type::number_int:	return lhs.m_data.m_int < rhs.m_data.m_int;
			case value_type::number_uint:	return lhs.m_data.m_uint < rhs.m_data.m_uint;
			case value_type::number_float:	return lhs.m_data.m_float < rhs.m_data.m_float;
			case value_type::boolean:		return lhs.m_data.m_boolean < rhs.m_data.m_boolean;
		}
	}
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_int)
		return lhs.m_data.m_float < static_cast<element::float_type>(rhs.m_data.m_int);
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_float)
		return static_cast<element::float_type>(lhs.m_data.m_int) < rhs.m_data.m_float;
	else if (lhs_type == value_type::number_float and rhs_type == value_type::number_uint)
		return lhs.m_data.m_float < static_cast<element::float_type>(rhs.m_data.m_uint);
	else if (lhs_type == value_type::number_uint and rhs_type == value_type::number_float)
		return static_cast<element::float_type>(lhs.m_data.m_uint) < rhs.m_data.m_float;
	else if (lhs_type == value_type::number_int and rhs_type == value_type::number_uint)
		return lhs.m_data.m_int < static_cast<element::int_type>(rhs.m_data.m_uint);
	else if (lhs_type == value_type::number_uint and rhs_type == value_type::number_int)
		return static_cast<element::int_type>(lhs.m_data.m_uint) < rhs.m_data.m_int;
	
	return false;
}

// --------------------------------------------------------------------

void serialize(std::ostream& os, const element& v)
{
	switch (v.m_type) 
	{
		case json::value_type::array:
		{
			auto& a = *v.m_data.m_array;
			os << '[';
			for (size_t i = 0; i < a.size(); ++i)
			{
				serialize(os, a[i]);
				if (i + 1 < a.size())
					os << ',';
			}
			os << ']';
			break;
		}

		case json::value_type::boolean:
			os << std::boolalpha << v.m_data.m_boolean;
			break;

		case json::value_type::null:
			os << "null";
			break;

		case json::value_type::number_float:
			os << v.m_data.m_float;
			break;

		case json::value_type::number_int:
			os << v.m_data.m_int;
			break;

		case json::value_type::number_uint:
			os << v.m_data.m_uint;
			break;

		case json::value_type::object:
		{
			os << '{';
			bool first = true;
			for (auto& kv: *v.m_data.m_object)
			{
				if (not first)
					os << ',';
				os << '"' << kv.first << "\":";
				serialize(os, kv.second);
				first = false;
			}
			os << '}';
			break;
		}

		case json::value_type::string:
			os << '"';
			
			for (char c: *v.m_data.m_string)
			{
				switch (c)
				{
					case '\"':	os << "\\\""; break;
					case '\\':	os << "\\\\"; break;
					case '/':	os << "\\/"; break;
					case '\b':	os << "\\b"; break;
					case '\n':	os << "\\n"; break;
					case '\r':	os << "\\r"; break;
					case '\t':	os << "\\t"; break;
					default:	if (c <  0x0020)
								{
									static const char kHex[17] = "0123456789abcdef";
									os << "\\00" << kHex[(c >> 4) & 0x0f] << kHex[c & 0x0f];
								}
								else	
									os << c;
								break;
				}
			}
			
			os << '"';
			break;
	}
}

// void serialize(std::ostream& os, const element& v, int indent, int level)
// {
//     switch (v.m_type)
//     {
//         case value_type::array:
//         {
//             auto& a = *v.m_data.m_array;
//             if (a.empty())
//                 os << std::setw(level * indent) << "[]" << std::endl;
//             else
//             {
//                 os << std::setw(level * indent) << '[' << std::endl;
//                 for (size_t i = 0; i < a.size(); ++i)
//                 {
//                     serialize(os, a[i], indent, level + 1);
//                     if (i + 1 < a.size())
//                         os << ',';
//                     os << std::endl;
//                 }
//                 os << std::setw(level * indent) << ']';
//             }
//             break;
//         }

//         case value_type::boolean:
//         {
//             auto& b = *v.m_data.m_boolean;
//             os << std::setw(level * indent) << std::boolalpha << b;
//             break;
//         }

//         case value_type::null:
//             os << std::setw(level * indent) << "null";
//             break;

//         case value_type::number_float:
//             os << std::setw(level * indent) << v.m_data.m_float;
//             break;

//         case value_type::number_int:
//             os << std::setw(level * indent) << v.m_data.m_int;
//             break;

//         case value_type::number_uint:
//             os << std::setw(level * indent) << v.m_data.m_uint;
//             break;

//         case value_type::object:
//         {
//             auto& o = *v.m_data.m_object;
//             if (o.empty())
//                 os << std::setw(level * indent) << "{}" << std::endl;
//             else
//             {
//                 os << std::setw(level * indent) << '{' << std::endl;
//                 for (size_t i = 0; i < a.size(); ++i)
//                 {


//                     os << 
//                     serialize(os, a[i], indent, level + 1);
//                     if (i + 1 < a.size())
//                         os << ',';
//                     os << std::endl;
//                 }
//                 os << std::setw(level * indent) << ']';
//             }
//             break;
//         }

//         case value_type::string:
//             os << std::setw(level * indent) << *v.m_data.m_string;
//             break;
//     }
// }

// --------------------------------------------------------------------

element parse(const std::string& s)
{
	return element();
}

std::ostream& operator<<(std::ostream& os, const element& v)
{
	// int indentation = os.width();
	// os.width(0);

	serialize(os, v);

	return os;
}

} // namespace zeep