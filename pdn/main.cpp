#include <iostream>
#include <string_view>

#include <pdn/spdn.h>

namespace
{
	std::string_view as_utf8(const pdn::type::u8string& s) noexcept
	{
		return std::string_view{ reinterpret_cast<const char*>(s.data()), s.size() };
	}
}

int main()
{
	using namespace std::string_view_literals;

	const auto spdn_content = u8R"(hello: "Hello world!")"sv;
	const auto parsed = pdn::parse(spdn_content, pdn::utf8_tag);
	if (!parsed)
	{
		std::cerr << "parse failed: " << parsed.what() << '\n';
		return 1;
	}

	const auto& dom = *parsed;
	std::cout << as_utf8(dom[u8"hello"sv].as_string()) << '\n';
	std::cout << as_utf8(pdn::make_u8serializer().serialize(dom.as_object())) << '\n';
	return 0;
}
