#include <iostream>

#include <zeep/json/element.hpp>

template<typename T>
struct Foo
{
	void bar(float);
};

int main()
{
	try
	{
		using zeep::json;

		// json x = R"({
		// 	"a": 1.1
		// })"_json;

		// x["aap"] = 1;
		// x["noot"] = vector<int>({ 1, 2, 3 });

		// json o = {
		// 	{ "a": 1 },
		// 	{ "b": "aap" },
		// 	{ "c:" { "sub": 1} },
		// 	[ 1, 2, "3", nullptr ]
		// };

		json j_null;
		json j_array(json::value_type::array);
		json j_object(json::value_type::object);
		json j_string(json::value_type::string);
		json j_number(json::value_type::number_int);
		json j_bool(json::value_type::boolean);

		json j_string2("aap");
		json j_number2(1.0f);
		json j_number3(2L);
		json j_number4(3UL);

		json j_bool2(true);

		json j_array2(std::vector<int>{ 1, 2, 3});
		json j_array3({ 1, 2, 3});

		std::map<std::string,int> m = {
			{ "een", 1 }, { "twee", 2 }
		};
		json j_object2(m);

		json j_array4(10, j_object2);

		static_assert(std::experimental::is_detected_exact
			<void, zeep::detail::to_element_function, zeep::element_serializer<int,void>, zeep::element&, int>::value, "moet!");
		static_assert(not zeep::detail::is_element<int>::value, "wowo");
		static_assert(zeep::detail::is_element<decltype(j_null)>::value, "wowo");
		static_assert(zeep::detail::has_to_element<bool>::value, "Oeps");

		std::cout << j_null << std::endl
				  << j_array << std::endl
				  << j_array2 << std::endl
				  << j_array3 << std::endl
				  << j_object << std::endl
				  << j_object2 << std::endl
				  << j_string << std::endl
				  << j_string2 << std::endl
				  << j_number << std::endl
				  << j_number2 << std::endl
				  << j_number3 << std::endl
				  << j_number4 << std::endl
				  << j_array4 << std::endl

				  << j_bool << std::endl
				  << j_bool2 << std::endl
				  ;


		// get

		bool b = *j_bool2.get_ptr<bool*>();
		assert(b == true);

		std::string s = *j_string2.get_ptr<std::string*>();
		assert(s == "aap");

		s = j_string2.get<std::string>();

		// assert(j_string2.get<std::string>() == "aap");

		std::cout << std::endl;
		
		std::cout << j_number2.get<float>() << std::endl
				  << j_number3.get<float>() << std::endl
				  << j_number4.get<float>() << std::endl
				  << std::endl;
		

		auto a = j_array3.get<std::vector<float>>();

		for (float f: a)
			std::cout << f << std::endl;

		std::cout << std::endl
				  << j_object2["een"] << std::endl
				  << j_array4[2]["twee"] << std::endl
				  << std::endl;
		

		for (auto[key, value]: j_object2.items())
		{
			std::cout << "key: " << key << " value: " << value << std::endl;
		}

	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		exit(1);
	}

	return 0;
}