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

class json_value;

template<typename,typename>
struct json_serializer;

namespace detail
{

enum class value_type : std::uint8_t
{
	null,
	object,
	array,
	string,
	number_int,
	number_uint,
	number_float,
	boolean
};

template<typename T>
class value_reference
{
public:
	using value_type = T;

	value_reference(value_type&& value)
		: m_owned(std::move(value)), m_reference(&m_owned), m_rvalue(true) {}

	value_reference(const value_type& value)
		: m_reference(const_cast<value_type*>(&value)), m_rvalue(false) {}
	
	value_reference(std::initializer_list<value_reference> init)
		: m_owned(init), m_reference(&m_owned), m_rvalue(true) {}
	
    template <typename... Args, std::enable_if_t<std::is_constructible<value_type, Args...>::value, int> = 0>
    value_reference(Args&& ... args)
        : m_owned(std::forward<Args>(args)...), m_reference(&m_owned), m_rvalue(true) {}

    // class should be movable only
    value_reference(value_reference&&) = default;
    value_reference(const value_reference&) = delete;
    value_reference& operator=(const value_reference&) = delete;
    value_reference& operator=(value_reference&&) = delete;
    ~value_reference() = default;

    value_type data() const
    {
        if (m_rvalue)
            return std::move(*m_reference);
        return *m_reference;
    }

    value_type const& operator*() const
    {
        return *static_cast<value_type const*>(m_reference);
    }

    value_type const* operator->() const
    {
        return static_cast<value_type const*>(m_reference);
    }

private:
	mutable value_type m_owned = nullptr;
	value_type* m_reference = nullptr;
	bool m_rvalue;
};

template<value_type> struct constructor {};

template<typename> struct is_json_value : std::false_type {};
template<> struct is_json_value<json_value> : std::true_type {};

template <typename T>
using mapped_type_t = typename T::mapped_type;

template <typename T>
using key_type_t = typename T::key_type;

template <typename T>
using value_type_t = typename T::value_type;

template <typename T>
using iterator_t = typename T::iterator;

template <typename T, typename... Args>
using to_json_function = decltype(T::to_json(std::declval<Args>()...));

template<typename T, typename = void>
struct has_to_json : std::false_type {};

template<typename T>
struct has_to_json<T, std::enable_if_t<not is_json_value<T>::value>>
{
	using serializer = json_serializer<T, void>;
	static constexpr bool value =
		std::experimental::is_detected_exact<void, to_json_function, serializer, json_value&, T>::value;
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
        std::is_constructible<json_value, typename T::value_type>::value;
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
		std::is_constructible<json_value,typename map_t::mapped_type>::value; 
};

template<typename T, typename = void>
struct is_compatible_type_impl : std::false_type {};

template<typename T>
struct is_compatible_type_impl<T, std::enable_if_t<is_complete_type<T>::value>>
{
	static constexpr bool value = has_to_json<T>::value;
};

template<typename T>
struct is_compatible_type : is_compatible_type_impl<T> {};


template<>
struct constructor<value_type::boolean>
{
	template<typename J>
	static void construct(J& j, bool b)
	{
		j.m_type = value_type::boolean;
		j.m_data = b;
		j.validate();
	}
};

template<>
struct constructor<value_type::string>
{
	template<typename J>
	static void construct(J& j, const std::string& s)
	{
		j.m_type = value_type::string;
		j.m_data = s;
		j.validate();
	}

	template<typename J>
	static void construct(J& j, std::string&& s)
	{
		j.m_type = value_type::string;
		j.m_data = std::move(s);
		j.validate();
	}
};

template<>
struct constructor<value_type::number_float>
{
	template<typename J>
	static void construct(J& j, double d)
	{
		j.m_type = value_type::number_float;
		j.m_data = d;
		j.validate();
	}
};

template<>
struct constructor<value_type::number_int>
{
	template<typename J>
	static void construct(J& j, int64_t i)
	{
		j.m_type = value_type::number_int;
		j.m_data = i;
		j.validate();
	}
};

template<>
struct constructor<value_type::number_uint>
{
	template<typename J>
	static void construct(J& j, uint64_t u)
	{
		j.m_type = value_type::number_uint;
		j.m_data = u;
		j.validate();
	}
};

template<>
struct constructor<value_type::array>
{
	template<typename J>
	static void construct(J& j, const typename J::array_type& arr)
	{
		j.m_type = value_type::array;
		j.m_data = arr;
		j.validate();
	}

	template<typename J>
	static void construct(J& j, typename J::array_type&& arr)
	{
		j.m_type = value_type::array;
		j.m_data = std::move(arr);
		j.validate();
	}

	template<typename J>
	static void construct(J& j, const std::vector<bool>& arr)
	{
		j.m_type = value_type::array;
		j.m_data = value_type::array;
		j.m_data.m_array->reserve(arr.size());
		std::copy(arr.begin(), arr.end(), std::back_inserter(*j.m_data.m_array));
		j.validate();
	}

	template<typename J, typename T,
		typename std::enable_if_t<std::is_convertible<T, json_value>::value, int> = 0>
	static void construct(J& j, const std::vector<T>& arr)
	{
		j.m_type = value_type::array;
		j.m_data = value_type::array;
		j.m_data.m_array->resize(arr.size());
		std::copy(arr.begin(), arr.end(), j.m_data.m_array->begin());
		j.validate();
	}
};

template<>
struct constructor<value_type::object>
{
	template<typename J>
	static void construct(J& j, const typename J::object_type& obj)
	{
		j.m_type = value_type::object;
		j.m_data = obj;
		j.validate();
	}

	template<typename J>
	static void construct(J& j, typename J::object_type&& obj)
	{
		j.m_type = value_type::object;
		j.m_data = std::move(obj);
		j.validate();
	}

    template<typename J, typename M,
             std::enable_if_t<not std::is_same<M, typename J::object_type>::value, int> = 0>
    static void construct(J& j, const M& obj)
    {
        using std::begin;
        using std::end;

        j.m_type = value_type::object;
        j.m_data.m_object = j.template create<typename J::object_type>(begin(obj), end(obj));
        j.validate();
    }

};

template<typename T, std::enable_if_t<std::is_same<T, bool>::value, int> = 0>
void to_json(json_value& v, T b)
{
	constructor<value_type::boolean>::construct(v, b);
}

template<typename T, std::enable_if_t<std::is_constructible<T, std::string>::value, int> = 0>
void to_json(json_value& v, const T& s)
{
	constructor<value_type::string>::construct(v, s);
}

inline void to_json(json_value& v, std::string&& s)
{
	constructor<value_type::string>::construct(v, std::move(s));
}

template<typename T, std::enable_if_t<std::is_floating_point<T>::value, int> = 0>
void to_json(json_value& v, T f)
{
	constructor<value_type::number_float>::construct(v, f);
}

template<typename T, std::enable_if_t<std::is_integral<T>::value and std::is_signed<T>::value and not std::is_same<T, bool>::value, int> = 0>
void to_json(json_value& v, T i)
{
	constructor<value_type::number_int>::construct(v, i);
}

template<typename T, std::enable_if_t<std::is_integral<T>::value and std::is_unsigned<T>::value and not std::is_same<T, bool>::value, int> = 0>
void to_json(json_value& v, T u)
{
	constructor<value_type::number_uint>::construct(v, u);
}

template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
void to_json(json_value& v, T e)
{
	using int_type = typename std::underlying_type<T>::type;
	constructor<value_type::number_int>::construct(v, static_cast<int_type>(e));
}

inline void to_json(json_value& j, const std::vector<bool>& v)
{
	constructor<value_type::array>::construct(j, v);
}

template<typename T, std::enable_if_t<is_array_type<T>::value, int> = 0>
void to_json(json_value& j, const T& arr)
{
	constructor<value_type::array>::construct(j, arr);
}

template<typename T, std::enable_if_t<std::is_convertible<json_value,T>::value, int> = 0>
void to_json(json_value& j, const std::valarray<T>& arr)
{
	constructor<value_type::array>::construct(j, std::move(arr));
}

template<typename J>
void to_json(json_value& j, const typename J::array_type& arr)
{
	constructor<value_type::array>::construct(j, std::move(arr));
}

template<typename J, typename T, size_t N,
	std::enable_if_t<not std::is_constructible<typename J::string_type, const T(&)[N]>::value, int> = 0>
void to_json(J& j, const T(&arr)[N])
{
	constructor<value_type::array>::construct(j, std::move(arr));
} 

template<typename T, std::enable_if_t<is_object_type<json_value,T>::value, int> = 0>
void to_json(json_value& j, const T& obj)
{
	constructor<value_type::object>::construct(j, std::move(obj));
}

template<typename J>
void to_json(J& j, const J& obj)
{
	constructor<value_type::object>::construct(j, std::move(obj));
}

struct to_json_fn
{
    template<typename T>
    auto operator()(json_value& j, T&& val) const noexcept(noexcept(to_json(j, std::forward<T>(val))))
    -> decltype(to_json(j, std::forward<T>(val)), void())
    {
        return to_json(j, std::forward<T>(val));
    }
};

namespace {

template<typename T>
struct static_const
{
    static constexpr T value{};
};

constexpr const auto& to_json = static_const<zeep::detail::to_json_fn>::value;

}

}

template<typename,typename>
struct json_serializer
{
	template<typename T>
	static auto to_json(json_value& j, T&& v)
		noexcept(noexcept(::zeep::detail::to_json(j, std::forward<T>(v))))
		-> decltype(::zeep::detail::to_json(j, std::forward<T>(v)))
	{
		::zeep::detail::to_json(j, std::forward<T>(v));
	}
};

class json_value
{
public:
	using value_type = detail::value_type;
	typedef std::map<std::string,json_value>	object_type;
	typedef std::vector<json_value>				array_type;
	typedef std::string							string_type;

    using initializer_list_t = std::initializer_list<detail::value_reference<json_value>>;

    template<value_type> friend struct detail::constructor;

	/// empty constructor with a certain type
	json_value(value_type t)
		: m_type(t), m_data(t)
	{
		validate();
	}

	/// default constructor
	json_value(std::nullptr_t = nullptr)
		: json_value(value_type::null)
	{
		validate();
	}

	json_value(const json_value& j)
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

	json_value(json_value&& j)
		: m_type(std::move(j.m_type)), m_data(std::move(j.m_data))
	{
		j.validate();

		j.m_type = value_type::null;
		j.m_data = {};

		validate();
	}

	json_value(const detail::value_reference<json_value>& r)
		: json_value(r.data())
	{
		validate();
	}

	template<typename T,
		typename U = typename std::remove_cv<typename std::remove_reference<T>::type>::type,
		std::enable_if_t<not std::is_same<U,json_value>::value and detail::is_compatible_type<T>::value, int> = 0>
	json_value(T&& v) 
	{
		json_serializer<U,void>::to_json(*this, std::forward<T>(v));
	}
 
	json_value(initializer_list_t init)
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

	json_value(size_t cnt, const json_value& v)
		: m_type(value_type::array)
	{
		m_data.m_array = create<array_type>(cnt, v);
		validate();
	}

	json_value& operator=(json_value j) noexcept(
        std::is_nothrow_move_constructible<value_type>::value and
        std::is_nothrow_move_assignable<value_type>::value and
        std::is_nothrow_move_constructible<json_value>::value and
        std::is_nothrow_move_assignable<json_value>::value);

	~json_value()
	{
		validate();
		m_data.destroy(m_type);
	}

	constexpr bool is_null() const noexcept						{ return m_type == value_type::null; }
	constexpr bool is_object() const noexcept					{ return m_type == value_type::object; }
	constexpr bool is_array() const noexcept					{ return m_type == value_type::array; }
	constexpr bool is_string() const noexcept					{ return m_type == value_type::string; }
	constexpr bool is_number() const noexcept					{ return is_number_int() or is_number_uint() or is_number_float(); }
	constexpr bool is_number_int() const noexcept				{ return m_type == value_type::number_int; }
	constexpr bool is_number_uint() const noexcept				{ return m_type == value_type::number_uint; }
	constexpr bool is_number_float() const noexcept				{ return m_type == value_type::number_float; }
	constexpr bool is_true() const noexcept						{ return is_boolean() and m_data.m_boolean == true; }
	constexpr bool is_false() const noexcept					{ return is_boolean() and m_data.m_boolean == false; }
	constexpr bool is_boolean() const noexcept					{ return m_type == value_type::boolean; }

	std::string type_name() const;

	struct iterator : public std::iterator<std::bidirectional_iterator_tag, json_value>
	{

	};



	template<typename... Args>
	std::pair<iterator,bool> emplace(Args&&... args)
	{
		if (is_null())
		{
			m_type = value_type::object;
			m_data = value_type::object;
		}
		else if (not is_object())
			throw std::runtime_error("Cannot emplace with json value of type " + type_name());
		
		validate();

		auto r = m_data.m_object->emplace(std::forward<Args>(args)...);
		auto i = begin();

		// TODO: set i.iter to r.first

		return { i, r.second };
	}

	iterator begin();
	iterator end();



private:

	union json_data
	{
		object_type*	m_object;
		array_type*		m_array;
		string_type*	m_string;
		int64_t			m_int;
		uint64_t		m_uint;
		double			m_float;
		bool			m_boolean;

		json_data() = default;
		json_data(bool v) noexcept : m_boolean(v) {}
		json_data(int64_t v) noexcept : m_int(v) {}
		json_data(uint64_t v) noexcept : m_uint(v) {}
		json_data(double v) noexcept : m_float(v) {}
		json_data(value_type t)
		{
			switch (t)
			{
				case value_type::array:			m_array = create<array_type>(); 	break;
				case value_type::boolean:		m_boolean = false;					break;
				case value_type::null:			m_object = nullptr;					break;
				case value_type::number_float:	m_float = 0;						break;
				case value_type::number_int:	m_int = 0;							break;
				case value_type::number_uint:	m_uint = 0;							break;
				case value_type::object:		m_object = create<object_type>(); 	break;
				case value_type::string:		m_string = create<string_type>();	break;
			}
		}
		json_data(const object_type& v)		{ m_object = create<object_type>(v); }
		json_data(object_type&& v)			{ m_object = create<object_type>(std::move(v)); }
		json_data(const string_type& v)		{ m_string = create<string_type>(v); }
		json_data(string_type&& v)			{ m_string = create<string_type>(std::move(v)); }
		json_data(const array_type& v)		{ m_array = create<array_type>(v); }
		json_data(array_type&& v)			{ m_array = create<array_type>(std::move(v)); }

		void destroy(value_type t) noexcept
		{
			switch (t)
			{
				case value_type::object:
				{
					std::allocator<object_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_object);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_object, 1);
					break;
				}

				case value_type::array:
				{
					std::allocator<array_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_array);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_array, 1);
					break;
				}

				case value_type::string:
				{
					std::allocator<string_type> alloc;
					std::allocator_traits<decltype(alloc)>::destroy(alloc, m_string);
					std::allocator_traits<decltype(alloc)>::deallocate(alloc, m_string, 1);
					break;
				}

				default:
					break;
			}
		}
	};

	template<typename T, typename... Args>
	static T* create(Args&&... args)
	{
        std::allocator<T> alloc;
        using AllocatorTraits = std::allocator_traits<std::allocator<T>>;

        auto deleter = [&](T * object)
        {
            AllocatorTraits::deallocate(alloc, object, 1);
        };

        std::unique_ptr<T, decltype(deleter)> object(AllocatorTraits::allocate(alloc, 1), deleter);
        AllocatorTraits::construct(alloc, object.get(), std::forward<Args>(args)...);
        assert(object != nullptr);
        return object.release();
	}

	// size_t size() const;

	// json_value& operator[](size_t index);
	// json_value& operator[](const std::string& name);

	friend std::ostream& operator<<(std::ostream& os, const json_value& v);
	friend void serialize(std::ostream& os, const json_value& data);
	// friend void serialize(std::ostream& os, const json_value& data, int indent, int level = 0);

private:

	void validate() const
	{
        assert(m_type != value_type::object or m_data.m_object != nullptr);
        assert(m_type != value_type::array or m_data.m_array != nullptr);
        assert(m_type != value_type::string or m_data.m_string != nullptr);
	}

	value_type	m_type;
	json_data	m_data;
};

using json = json_value;


} // namespace zeep

zeep::json_value operator""_json(const char* s, size_t len);
