#include <iostream>

#include "spdn.h"

#include "outu8sv.h"

int main()
{
	auto result = pdn::parse("hello.spdn", pdn::utf8_tag);

	if (!result)
	{
		std::cerr << "failed in open or parse hello.spdn: " << result.what() << "\n";
		return 1;
	}

	const auto& entity = *result;
	const auto& say = entity[u8"say"]; // query the content of "say"
	std::cout << say.as_string() << "\n"; // use "say" as a string and print the content of "say"
}
