#include <iostream>
#include <string>

#include "spdn.h"

std::ostream& operator<<(std::ostream& os, const pdn::type::u8string& s)
{
	return os << std::string_view{ reinterpret_cast<const char*>(s.c_str()), s.size() };
}

int main() try
{
	using namespace std::string_view_literals;
	auto spdn_content = u8R"(hello: "Hello world!")"sv;
	auto e = pdn::parse(spdn_content, pdn::utf8_tag);
	std::cout << e[u8"hello"sv].as_string() << "\n";
	std::cout << pdn::make_u8serializer().serialize(e.as_object()) << "\n";
}
catch (std::exception& e)
{
	std::cerr << e.what() << "\n";
}
catch (...)
{
	std::cerr << "unknown exception\n";
}
