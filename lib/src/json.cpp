//               Copyright Maarten L. Hekkelman.
//   Distributed under the Boost Software License, Version 1.0.
//      (See accompanying file LICENSE_1_0.txt or copy at
//            http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>

#include <zeep/json/json.hpp>

namespace zeep
{





struct json_value_impl_base
{
	virtual ~json_value_impl_base() {}

    virtual void write(std::ostream& os) = 0;
};

template<typename impl>
struct json_value_impl_t : public json_value_impl_base
{
};

template<typename T>
struct json_value_impl : public json_value_impl_base {};

template<>
struct json_value_impl<std::string> : public json_value_impl_base
{
	std::string m_value;

    virtual void write(std::ostream& os)
    {
        os << '"';
        
        for (char c: m_value)
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
    }
};

template<>
struct json_value_impl<long double> : public json_value_impl_base
{
	long double m_value;

    virtual void write(std::ostream& os)
    {
        os << std::defaultfloat << m_value;
    }
};

template<>
struct json_value_impl<bool> : public json_value_impl_base
{
	bool m_value;

    virtual void write(std::ostream& os)
    {
        os << std::boolalpha << m_value;
    }
};

// template<>
// struct json_value_impl<array> : public json_value_impl_base
// {
// 	array m_value;

//     virtual void write(std::ostream& os)
//     {
//         os << '[';
//         for (auto& v: m_value)
//             os << v;
//         os << ']';
//     }
// };

template<>
struct json_value_impl<std::nullptr_t> : public json_value_impl_base
{
    virtual void write(std::ostream& os)
    {
        os << "null";
    }
};

// --------------------------------------------------------------------


void serialize(std::ostream& os, const json_value& v)
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

// void serialize(std::ostream& os, const json_value& v, int indent, int level)
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

json_value& json_value::operator=(json_value j) noexcept(
        std::is_nothrow_move_constructible<value_type>::value and
        std::is_nothrow_move_assignable<value_type>::value and
        std::is_nothrow_move_constructible<json_value>::value and
        std::is_nothrow_move_assignable<json_value>::value)
{
    j.validate();

    using std::swap;

    swap(m_type, j.m_type);
    swap(m_data, j.m_data);

    validate();

    return *this; 
}

json_value parse(const std::string& s)
{
    return json_value();
}

std::ostream& operator<<(std::ostream& os, const json_value& v)
{
    // int indentation = os.width();
    // os.width(0);

    serialize(os, v);

    return os;
}

} // namespace zeep