#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>

#include "pdn_parse.h"
#include "pdn_data_entity.h"
#include "pdn_serializer.h"

void serializer_test()
{
	using namespace pdn;
	auto e_opt = parse("../test/features_test.spdn", utf_8_tag);
	if (!e_opt)
	{
		std::cerr << "failed in parse \"../test/features_test.spdn\"\n";
		return;
	}
	const auto& e = *e_opt;
	auto s = make_u8serializer().serialize(as_object(e));
	{
		std::ofstream f("../test/serialize_test.spdn");
		if (!f.is_open())
		{
			std::cerr << "failed in open \"../test/serialize_test.spdn\"\n";
			return;
		}
		f.write((const char*)s.data(), s.size());
	}
	auto copied_e_opt = parse("../test/serialize_test.spdn", utf_8_tag);
}

struct my_date
{
	int y{};
	int m{};
	int d{};

	explicit my_date(const pdn::u8entity_cref& e)
	{
		from_entity(e);
	}
	void from_entity(const pdn::u8entity_cref& e)
	{
		using namespace pdn;
		y = as_int(e[u8"y"], auto_int_tag);
		m = as_int(e[u8"m"], auto_int_tag);
		d = as_int(e[u8"d"], auto_int_tag);
	}
	auto to_entity() const -> pdn::types::object<char8_t>
	{
		using namespace pdn;
		types::object<char8_t> o;
		o[u8"y"] = y;
		o[u8"m"] = m;
		o[u8"d"] = d;
		return o;
	}
};

int main()
{
	serializer_test();

	using namespace std::literals::string_view_literals;
	auto pdn_src = u8R"(// spdn source

`I tell U` [
	"This software was completed on the evening of December 24, 2024, and I am very happy now.",
	"Next, I will be busy with some testing-related tasks, and then this project will be almost finished.",
]

date {
	y 2024;
	m 12;
	d 24;
}

`Am I happy now` </ definitely /> @true;

num   : 123;
n_num : -321;
bool  : @true;
str   : "string";
)"sv;

	using namespace pdn;

	auto dom = parse(pdn_src, utf_8_tag);

	auto cref = const_refer<char8_t>{ dom };

	for (const auto& s : as_list(cref[u8"I tell U"sv]))
	{
		std::cout << (const char*)as_string(s).c_str() << "\n";
	}

	std::cout << "Am I happy now? " << std::boolalpha << as_bool(cref[u8"Am I happy now"sv]) << "\n";

	if (get<types::auto_int>(dom[u8"num"]) == 123)
	{
		std::cout << "num = 123\n";
	}

	if (auto p = get_ptr<types::auto_int>(cref[u8"n_num"]))
	{
		std::cout << "n_num = " << *p << "\n";
	}

	if (auto opt = get_optional<types::boolean>(cref[u8"bool"]))
	{
		std::cout << "bool = " << std::boolalpha << *opt << "\n";
	}

	my_date date(cref[u8"date"]);
	std::cout << (const char*)pdn::make_u8serializer().serialize(date.to_entity()).c_str() << "\n";
}
