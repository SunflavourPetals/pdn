#include <iostream>
#include <iomanip>
#include <string>

#include "pdn_parse.h"
#include "pdn_data_entity.h"

int main()
{
	using namespace std::literals::string_view_literals;
	auto pdn_src = u8R"(// spdn source

`I tell U` [
	"This software was completed on the evening of December 24, 2024, and I am very happy now.",
	"Next, I will be busy with some testing-related tasks, and then this project will be almost finished.",
]

date {
	y 2024;
	m 12;
	d 24;
}

`Am I happy now` </ definitely /> @true;

num   : 123;
n_num : -321;
bool  : @true;
str   : "string";
)"sv;

	using namespace pdn;

	auto dom = parse(pdn_src, utf_8_tag);

	auto cref = const_refer<char8_t>{ dom };

	for (const auto& s : as_list(cref[u8"I tell U"sv]))
	{
		std::cout << (const char*)as_string(s).c_str() << "\n";
	}

	std::cout << "Am I happy now? " << std::boolalpha << as_bool(cref[u8"Am I happy now"sv]) << "\n";

	if (get<types::cppint>(dom[u8"num"]) == 123)
	{
		std::cout << "num = 123\n";
	}

	if (auto p = get_ptr<types::cppint>(cref[u8"n_num"]))
	{
		std::cout << "n_num = " << *p << "\n";
	}

	if (auto opt = get_optional<types::boolean>(cref[u8"bool"]))
	{
		std::cout << "bool = " << std::boolalpha << *opt << "\n";
	}
}
