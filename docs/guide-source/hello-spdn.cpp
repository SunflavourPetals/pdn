#include <iostream>

#include <pdn/spdn.h>

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
	const auto& greeting = entity[u8"greeting"]; // query the content of "greeting"
	std::cout << greeting.as_string() << "\n"; // use "greeting" as a string and print the content of "greeting"
}
