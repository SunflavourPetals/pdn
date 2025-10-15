#include <iostream>

#include "pdn_parse.h"
#include "pdn_data_entity.h"

#include "outu8sv.h"

int main()
{
	using namespace pdn;
	
	auto entity_opt = parse("hello.spdn", utf8_tag);
	
	if (!entity_opt)
	{
		std::cerr << "failed in parse hello.spdn\n";
		return 0;
	}
	auto& entity = *entity_opt;

	auto& say = entity[u8"say"];
	try
	{
		say[u8""];
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
	try
	{
		entity[u8"name"];
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}

	{
		auto e_ref = entity.cref();
		auto say_ref = e_ref[u8"say"];
		auto x = say_ref[u8"x"];
		if (!x) std::cout << "get null ref from say_ref[u8\"x\"]\n";
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
