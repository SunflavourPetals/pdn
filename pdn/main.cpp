#include <iostream>
#include <format>
#include <string>
#include <fstream>
#include <cassert>

#include "pdn_parse.h"
#include "pdn_data_entity.h"
#include "pdn_serializer.h"

void serializer_test()
{
	using namespace pdn;
	auto e_opt = parse("../test/features_test.spdn", utf8_tag);
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

	auto copied_e_opt = parse("../test/serialize_test.spdn", utf8_tag);
	if (!copied_e_opt)
	{
		std::cerr << "failed in parse \"../test/serialize_test.spdn\"\n";
	}
}

void access_test()
{
	using namespace std::string_view_literals;
	const auto src = u8R"(//source
auto_int : int 0;
auto_uint:uint 1;
i8 :i8  - 8;
i16:i16 -16;
i32:i32 -32;
i64:i64 -64;
u8 :u8    8;
u16:u16  16;
u32:u32  32;
u64:u64  64;
f32:f32 1.0;
f64:f64 2.0;
bool @true;
char 'C';
str "string";
object {
	m1 1;
	m2 "member";
	m3 [1, 2, 3];
}
list [
	{ num 0 },
	[ 0 ],
	0,
	0.0,
	'0',
	@false,
]
)"sv;

	using namespace pdn;

	const auto& e = parse(src, utf8_tag);
	const auto r = e.ref();

	using namespace pdn::types;

	// get
	(void)get<auto_int>(e[u8"auto_int"]);
	(void)get<object<char8_t>>(e[u8"object"]);

	auto test_i64 = get<i64>(e[u8"i64"]);
	assert(test_i64 == -64);
	// get_ptr
	auto test_list_p = get_ptr<list<char8_t>>(e[u8"list"]);
	assert(test_list_p != nullptr);
	auto test_nonexistence_p = get_ptr<list<char8_t>>(r[u8"nonexistence"]);
	assert(test_nonexistence_p == nullptr);
	// get_optional
	auto test_auto_uint_opt = get_optional<auto_uint>(e[u8"auto_uint"]);
	assert(test_auto_uint_opt && (*test_auto_uint_opt == 1));
	auto test_nonexistence_opt = get_optional<i32>(r[u8"nonexistence"]);
	assert(static_cast<bool>(test_nonexistence_opt) == false);
	// as
	auto test_as_u32 = as_uint(e[u8"u32"], u32_tag);
	static_assert(::std::same_as<decltype(test_as_u32), u32>);
	assert(test_as_u32 == 32);
	auto test_as_i8 = as_int(r[u8"u64"], i8_tag);
	static_assert(::std::same_as<decltype(test_as_i8), i8>);
	assert(test_as_i8 == 64);
	const auto& test_as_string = as_string(r[u8"char"]);
	assert(test_as_string == string<char8_t>{});
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

int main() try
{
//	serializer_test();
	access_test();

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

	auto a = entity<char8_t>{};
	auto dom = parse(pdn_src, utf8_tag);

	auto cref = const_refer<char8_t>{ dom };

	for (const auto& s : as_list(cref[u8"I tell U"sv]))
	{
		std::cout << (const char*)as_string(s).c_str() << "\n";
	}

	std::cout << std::format("Am I happy now? {}!\n", as_bool(cref[u8"Am I happy now"sv]) ? "yes" : "no");

	if (get<types::auto_int>(dom[u8"num"]) == 123)
	{
		std::cout << "num = 123\n";
	}

	if (auto p = get_ptr<types::auto_int>(cref[u8"n_num"]))
	{
		std::cout << std::format("n_num = {}\n", *p);
	}

	if (auto opt = get_optional<types::boolean>(cref[u8"bool"]))
	{
		std::cout << std::format("bool = {}\n", *opt);
	}

	my_date date(cref[u8"date"]);
	std::cout << (const char*)pdn::make_u8serializer().serialize(date.to_entity()).c_str() << "\n";
}
catch (std::exception& e)
{
	std::cerr << e.what() << "\n";
}
catch (...)
{
	std::cerr << "unknown exception\n";
}
