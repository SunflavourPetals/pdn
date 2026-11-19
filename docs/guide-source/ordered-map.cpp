#include <cassert>
#include <iostream>
#include <string>
#include <string_view>

#define PDN_USER_OBJECT ::pdn::ordered_map
#include <pdn/pdn_ordered_map.h>
#include <pdn/spdn.h>

#include "outu8sv.h"

int main()
{
	using namespace std::literals;

	auto spdn_text = u8R"(zeta 1 alpha 2 mid 3)"sv;
	auto result = pdn::parse(spdn_text, pdn::utf8_tag);
	assert(result);

	// The object keeps the order in which the members were written.
	auto& object = result->as_object();
	std::u8string keys;
	for (const auto& [key, value] : object)
	{
		(void)value;
		keys += key;
		keys += u8',';
	}
	std::cout << "keys: " << keys << "\n";
	assert(keys == u8"zeta,alpha,mid,");

	// Serializing writes them back in the same order.
	auto text = pdn::make_u8serializer().serialize(object);
	std::cout << text;
	assert(text.find(u8"zeta") < text.find(u8"alpha"));
	assert(text.find(u8"alpha") < text.find(u8"mid"));

	std::cout << "object size: " << object.size() << "\n";
}
