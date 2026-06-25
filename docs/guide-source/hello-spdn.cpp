#include <iostream>
#include <exception>

#include "spdn.h"

#include "outu8sv.h"

int main() try
{
	auto entity_opt = pdn::parse("hello.spdn", pdn::utf8_tag);
	
	if (!entity_opt)
	{
		std::cerr << "failed in open or parse hello.spdn\n";
		return 0;
	}
	
	const auto& entity = *entity_opt; // this optional has value, get a reference and named it entity
	const auto& say = entity[u8"say"]; // query the content of "say"
	std::cout << say.as_string() << "\n"; // use "say" as a string and print the content of "say"
}
catch (std::exception& e)
{
    std::cerr << e.what() << "\n";
}
catch (...)
{
    std::cerr << "unknown exception\n";
}
