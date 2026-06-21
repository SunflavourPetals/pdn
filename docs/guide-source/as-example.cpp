#include <iostream>
#include <string>

#include "spdn.h"

#include "outu8sv.h"

int main()
{
	using namespace pdn;
	using namespace std::string_view_literals;

	auto e = parse(u8R"(
			f   : 1.25
			inf : @inf
			c   : '爱'
			nan : @nan
			s   : "string"
		)"sv, utf8_tag);

	std::cout << as_int(e[u8"f"]) << "\n"; // 1

	std::cout << e[u8"inf"].as_int() << "\n"; // 9223372036854775807

	std::cout << std::boolalpha << as_bool(e[u8"c"]) << "\n"; // true

	std::cout << as_int(e[u8"nan"]) << "\n"; // 0

	std::cout << as_string(e[u8"f"]) << "\n"; // *null string
}