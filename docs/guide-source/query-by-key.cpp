#include <iostream>
#include <string>
#include <exception>

#include "spdn.h"

#include "outu8sv.h"

int main()
{
	using namespace pdn;
	
	auto result = parse("hello.spdn", utf8_tag);
	
	if (!result)
	{
		std::cerr << "failed in parse hello.spdn: " << result.what() << "\n";
		return 1;
	}
	auto& entity = *result;

	auto& greeting = entity[u8"greeting"];
	try
	{
		greeting[u8""]; // (maybe)std::get: wrong index for variant
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
	try
	{
		entity[u8"name"]; // out_of_range
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}

	{
		auto e_ref = entity.cref();
		auto greeting_ref = e_ref[u8"greeting"];
		auto x = greeting_ref[u8"x"];
		if (!x) std::cout << "get null ref from greeting_ref[u8\"x\"]\n";
		auto y = x[u8"y"];
		if (!y) std::cout << "get null ref from x[u8\"y\"]\n";
		auto name = e_ref[u8"name"];
		if (name.has_value())
		{
			std::cout << "has an entity named \"name\"\n";
		}
		else
		{
			std::cout << "no entity named \"name\"\n";
		}
	}
}
