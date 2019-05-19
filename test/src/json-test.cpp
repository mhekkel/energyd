#include <iostream>

#include <zeep/json/json.hpp>

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

		json j_string2("aap");
		json j_number2(1.0f);
		json j_number3(2L);
		json j_number4(3UL);

		json j_array2(std::vector<int>{ 1, 2, 3});

		static_assert(std::experimental::is_detected_exact
			<void, zeep::detail::to_json_function, zeep::json_serializer<int,void>, zeep::json_value&, int>::value, "moet!");
		static_assert(not zeep::detail::is_json_value<int>::value, "wowo");
		static_assert(zeep::detail::is_json_value<decltype(j_null)>::value, "wowo");
		static_assert(zeep::detail::has_to_json<bool>::value, "Oeps");

		std::cout << j_null << std::endl
				  << j_array << std::endl
				  << j_array2 << std::endl
				  << j_object << std::endl
				  << j_string << std::endl
				  << j_string2 << std::endl
				  << j_number << std::endl
				  << j_number2 << std::endl
				  << j_number3 << std::endl
				  << j_number4 << std::endl
				  ;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		exit(1);
	}

	return 0;
}