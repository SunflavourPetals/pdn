#include <iostream>

#include "pdn_parse.h"
#include "pdn_data_entity.h"

#include "outu8sv.h"

int main()
{
	auto entity_opt = pdn::parse("hello.spdn", pdn::utf8_tag);
	
	if (!entity_opt)
	{
		std::cerr << "failed in parse hello.spdn\n";
		return 0;
	}
	
	const auto& entity = *entity_opt; // this optional has value, get a reference and named it entity
	const auto& say = entity[u8"say"]; // query the content of "say"
	std::cout << pdn::as_string(say) << "\n"; // use "say" as a string and print the content of "say"
}
