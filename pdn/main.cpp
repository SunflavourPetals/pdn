#include <iostream>
#include <format>
#include <string>
#include <fstream>
#include <cassert>

#include "spdn.h"

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

	[[maybe_unused]] auto test_i64 = get<i64>(e[u8"i64"]);
	assert(test_i64 == -64);
	// get_ptr
	[[maybe_unused]] auto test_list_p = get_ptr<list<char8_t>>(e[u8"list"]);
	assert(test_list_p != nullptr);
	[[maybe_unused]] auto test_nonexistence_p = get_ptr<list<char8_t>>(r[u8"nonexistence"]);
	assert(test_nonexistence_p == nullptr);
	// get_optional
	[[maybe_unused]] auto test_auto_uint_opt = get_optional<auto_uint>(e[u8"auto_uint"]);
	assert(test_auto_uint_opt && (*test_auto_uint_opt == 1));
	[[maybe_unused]] auto test_nonexistence_opt = get_optional<i32>(r[u8"nonexistence"]);
	assert(static_cast<bool>(test_nonexistence_opt) == false);
	// as
	[[maybe_unused]] auto test_as_u32 = as_uint(e[u8"u32"], u32_tag);
	static_assert(::std::same_as<decltype(test_as_u32), u32>);
	assert(test_as_u32 == 32);
	[[maybe_unused]] auto test_as_i8 = as_int(r[u8"u64"], i8_tag);
	static_assert(::std::same_as<decltype(test_as_i8), i8>);
	assert(test_as_i8 == 64);
	[[maybe_unused]] const auto& test_as_string = as_string(r[u8"char"]);
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

void as_series_member_test()
{
	using namespace std::literals;
	auto content = u8R"( // test as(member) series functions
int  : int 0;
i8 : i8 -8;
i16: i16 -16;
i32: i32 -32;
i64: i64 -64;
uint : uint 1;
u8 : u8 8;
u16: u16 16;
u32: u32 32;
u64: u64 64;
f32: f32 1.0;
f64: f64 2.0;
bool: @true;
char: 'C';
str: "string";
list: [
	1,
	2,
	3,
];
object: {
	m1 1;
	m2 "member";
	m3 [1, 2, 3];
};
)"sv;
	auto ex = pdn::parse(content, pdn::utf8_tag);
	auto ex16 = pdn::parse(content, pdn::utf16_tag);
	auto ex32 = pdn::parse(content, pdn::utf32_tag);
	// auto e = ex.cref();
	// auto e = ex.ref();
	auto& e = ex;
	constexpr auto lf = "\n";

	auto m1_it = e[u8"object"].as_object().find(u8"m1"sv);
	std::cout
		<< "as_series_member_test begin" << lf
		<< e[u8"int"].as_int(pdn::auto_int_tag) << lf
		<< (int)e[u8"i8"].as_int(pdn::i8_tag) << lf
		<< e[u8"i16"].as_int(pdn::i16_tag) << lf
		<< e[u8"i32"].as_int(pdn::i32_tag) << lf
		<< e[u8"i64"].as_int(pdn::i64_tag) << lf
		<< e[u8"uint"].as_uint(pdn::auto_uint_tag) << lf
		<< (unsigned)e[u8"u8"].as_uint(pdn::u8_tag) << lf
		<< e[u8"u16"].as_uint(pdn::u16_tag) << lf
		<< e[u8"u32"].as_uint(pdn::u32_tag) << lf
		<< e[u8"u64"].as_uint(pdn::u64_tag) << lf
		<< e[u8"f32"].as_fp(pdn::f32_tag) << lf
		<< e[u8"f64"].as_fp(pdn::f64_tag) << lf
		<< e[u8"bool"].as_bool() << lf
		<< (char)e[u8"char"].as_char().data()[0] << lf
		<< (const char*)e[u8"str"].as_string().c_str() << lf
		<< e[u8"list"].as_list()[0].as_int() << lf
		<< ((m1_it == e[u8"object"].as_object().cend()) ? "m1 not found"s : std::to_string(m1_it->second.as_uint())) << lf
		<< e[u8"object"][u8"m3"][0].as_fp() << lf
		<< "as_series_member_test end" << lf;
	[[maybe_unused]] auto test_u8s  = e[u8"str"].as_u8string();
	[[maybe_unused]] auto test_u16s = e[u8"str"].as_u16string();
	[[maybe_unused]] auto test_u32s = e[u8"str"].as_u32string();
}

void get_series_member_test()
{
	using namespace std::literals;
	auto content = u8R"( // test as(member) series functions
int  : int 0;
i8 : i8 -8;
i16: i16 -16;
i32: i32 -32;
i64: i64 -64;
uint : uint 1;
u8 : u8 8;
u16: u16 16;
u32: u32 32;
u64: u64 64;
f32: f32 1.0;
f64: f64 2.0;
bool: @true;
char: 'C';
str: "string";
list: [
	1,
	2,
	3,
];
object: {
	m1 1;
	m2 "member";
	m3 [1, 2, 3];
};
)"sv;
	auto ex = pdn::parse(content, pdn::utf8_tag);
	auto& e = ex;
	// auto e = ex.ref();
	// auto e = ex.cref();

	// for entity
	auto& o = e[u8"object"sv].get<pdn::types::object<char8_t>>();
	o[u8"m1"] = 123;

	// for ref
	// *e[u8"object"sv][u8"m1"].get_ptr<pdn::types::auto_int>() = 123;

	auto lf = "\n";
	std::cout
		<< "get_series_member_test begin" << lf
		
		<< e[u8"int"].get<pdn::types::auto_int>() << lf
		<< (int)e[u8"i8"].get<pdn::types::i8>() << lf
		<< e[u8"i16"].get<pdn::types::i16>() << lf
		<< e[u8"i32"].get<pdn::types::i32>() << lf
		<< e[u8"i64"].get<pdn::types::i64>() << lf
		<< e[u8"uint"].get<pdn::types::auto_uint>() << lf
		<< (unsigned)e[u8"u8"].get<pdn::types::u8>() << lf
		<< e[u8"u16"].get<pdn::types::u16>() << lf
		<< e[u8"u32"].get<pdn::types::u32>() << lf
		<< e[u8"u64"].get<pdn::types::u64>() << lf
		<< e[u8"f32"].get<pdn::types::f32>() << lf
		<< e[u8"f64"].get<pdn::types::f64>() << lf
		<< e[u8"bool"].get<pdn::types::boolean>() << lf
		<< (char)e[u8"char"].get<pdn::types::character<char8_t>>().data()[0] << lf
		<< (const char*)e[u8"str"].get<pdn::types::string<char8_t>>().c_str() << lf
		<< e[u8"list"].get<pdn::types::list<char8_t>>()[0].get<pdn::types::auto_int>() << lf
		<< e[u8"object"].get<pdn::types::object<char8_t>>()[u8"m1"].get<pdn::types::auto_int>() << lf
		
		// get_ptr vvv
		<< *e[u8"int"].get_ptr<pdn::types::auto_int>() << lf
		<< (int)*e[u8"i8"].get_ptr<pdn::types::i8>() << lf
		<< *e[u8"i16"].get_ptr<pdn::types::i16>() << lf
		<< *e[u8"i32"].get_ptr<pdn::types::i32>() << lf
		<< *e[u8"i64"].get_ptr<pdn::types::i64>() << lf
		<< *e[u8"uint"].get_ptr<pdn::types::auto_uint>() << lf
		<< (unsigned)*e[u8"u8"].get_ptr<pdn::types::u8>() << lf
		<< *e[u8"u16"].get_ptr<pdn::types::u16>() << lf
		<< *e[u8"u32"].get_ptr<pdn::types::u32>() << lf
		<< *e[u8"u64"].get_ptr<pdn::types::u64>() << lf
		<< *e[u8"f32"].get_ptr<pdn::types::f32>() << lf
		<< *e[u8"f64"].get_ptr<pdn::types::f64>() << lf
		<< *e[u8"bool"].get_ptr<pdn::types::boolean>() << lf
		<< (char)e[u8"char"].get_ptr<pdn::types::character<char8_t>>()->data()[0] << lf
		<< (const char*)e[u8"str"].get_ptr<pdn::types::string<char8_t>>()->c_str() << lf
		<< e[u8"list"].get_ptr<pdn::types::list<char8_t>>()->operator[](0).get<pdn::types::auto_int>() << lf
		<< e[u8"object"].get_ptr<pdn::types::object<char8_t>>() /*->operator[](u8"m1").get<pdn::types::auto_int>()*/ << lf
		// for entity test and ref test, comment it out when testing cref
		<< e[u8"object"].get_ptr<pdn::types::object<char8_t>>()->operator[](u8"m1").get<pdn::types::auto_int>() << lf
		// get_opt vvv
		<< *e[u8"int"].get_optional<pdn::types::auto_int>() << lf
		<< (int)*e[u8"i8"].get_optional<pdn::types::i8>() << lf
		<< *e[u8"i16"].get_optional<pdn::types::i16>() << lf
		<< *e[u8"i32"].get_optional<pdn::types::i32>() << lf
		<< *e[u8"i64"].get_optional<pdn::types::i64>() << lf
		<< *e[u8"uint"].get_optional<pdn::types::auto_uint>() << lf
		<< (unsigned)*e[u8"u8"].get_optional<pdn::types::u8>() << lf
		<< *e[u8"u16"].get_optional<pdn::types::u16>() << lf
		<< *e[u8"u32"].get_optional<pdn::types::u32>() << lf
		<< *e[u8"u64"].get_optional<pdn::types::u64>() << lf
		<< *e[u8"f32"].get_optional<pdn::types::f32>() << lf
		<< *e[u8"f64"].get_optional<pdn::types::f64>() << lf
		<< *e[u8"bool"].get_optional<pdn::types::boolean>() << lf
		<< (char)e[u8"char"].get_optional<pdn::types::character<char8_t>>()->data()[0] << lf
		// other
		<< ::std::boolalpha << e[u8"object"][u8"m2"].type_test<pdn::types::string<char8_t>>() << lf
		<< "get_series_member_test end" << lf;
}

int main() try
{
	as_series_member_test();
	get_series_member_test();
//	serializer_test();
	access_test();

	std::cout << "variant & entity size: " << sizeof(pdn::types::detail::entity_variant<char8_t>) << " " << sizeof(pdn::entity<char8_t>) << "\n";
	std::cout << "             ref size: " << sizeof(pdn::refer<char8_t>) << "\n";
	std::cout << "            cref size: " << sizeof(pdn::const_refer<char8_t>) << "\n";

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

	auto dom = parse(pdn_src, utf8_tag);

	auto cref = dom.cref();

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
