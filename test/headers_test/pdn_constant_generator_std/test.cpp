#include <cmath>
#include <cstdio>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>

#include <pdn/spdn.h>
#include <pdn/pdn_constant_generator_std.h>

namespace
{
	int fails = 0;

#define CHECK(condition) do { if (!(condition)) { std::printf("FAILED: %s (line %d)\n", #condition, __LINE__); ++fails; } } while (false)

	using u8_t = pdn::unicode::u8char_t;
	using u16_t = pdn::unicode::u16char_t;
	using u32_t = pdn::unicode::u32char_t;
	using u8s = pdn::unicode::u8string;
	using u8sv = pdn::unicode::u8string_view;
	using e8 = pdn::entity<u8_t>;

	auto gen(const char8_t* iden) -> ::std::optional<e8>
	{
		return pdn::constant_generator_std_function<u8_t>(u8s{ iden });
	}

	struct name_and_type
	{
		const char8_t* iden;
		pdn::type_code type;
	};

	// The 45 identifiers the reference requires, in the order of the table there.
	constexpr name_and_type standard_names[]
	{
		{ u8"i8_min", pdn::type_code::i8 },       { u8"i8_max", pdn::type_code::i8 },
		{ u8"i16_min", pdn::type_code::i16 },     { u8"i16_max", pdn::type_code::i16 },
		{ u8"i32_min", pdn::type_code::i32 },     { u8"i32_max", pdn::type_code::i32 },
		{ u8"i64_min", pdn::type_code::i64 },     { u8"i64_max", pdn::type_code::i64 },
		{ u8"u8_min", pdn::type_code::u8 },       { u8"u8_max", pdn::type_code::u8 },
		{ u8"u16_min", pdn::type_code::u16 },     { u8"u16_max", pdn::type_code::u16 },
		{ u8"u32_min", pdn::type_code::u32 },     { u8"u32_max", pdn::type_code::u32 },
		{ u8"u64_min", pdn::type_code::u64 },     { u8"u64_max", pdn::type_code::u64 },
		{ u8"int_min", pdn::type_code::i32 },     { u8"int_max", pdn::type_code::i32 },
		{ u8"uint_min", pdn::type_code::u32 },    { u8"uint_max", pdn::type_code::u32 },
		{ u8"true", pdn::type_code::boolean },    { u8"false", pdn::type_code::boolean },
		{ u8"e", pdn::type_code::f64 },           { u8"log2e", pdn::type_code::f64 },
		{ u8"log10e", pdn::type_code::f64 },      { u8"pi", pdn::type_code::f64 },
		{ u8"inv_pi", pdn::type_code::f64 },      { u8"inv_sqrtpi", pdn::type_code::f64 },
		{ u8"ln2", pdn::type_code::f64 },         { u8"ln10", pdn::type_code::f64 },
		{ u8"sqrt2", pdn::type_code::f64 },       { u8"sqrt3", pdn::type_code::f64 },
		{ u8"inv_sqrt3", pdn::type_code::f64 },   { u8"egamma", pdn::type_code::f64 },
		{ u8"phi", pdn::type_code::f64 },         { u8"infinity", pdn::type_code::f64 },
		{ u8"inf", pdn::type_code::f64 },         { u8"quiet_NaN", pdn::type_code::f64 },
		{ u8"qNaN", pdn::type_code::f64 },        { u8"qnan", pdn::type_code::f64 },
		{ u8"NaN", pdn::type_code::f64 },         { u8"nan", pdn::type_code::f64 },
		{ u8"signaling_NaN", pdn::type_code::f64 }, { u8"sNaN", pdn::type_code::f64 },
		{ u8"snan", pdn::type_code::f64 },
	};

#ifndef PDN_NO_EXTENSIONS
	// The six non-standard entries the reference documents as test-only extras.
	constexpr name_and_type extra_names[]
	{
		{ u8"\u03C0", pdn::type_code::f64 },
		{ u8"\u03B3", pdn::type_code::f64 },
		{ u8"\u03A6", pdn::type_code::f64 },
		{ u8"hello", pdn::type_code::string },
		{ u8"fib_10_list", pdn::type_code::list },
		{ u8"me_object", pdn::type_code::object },
	};
#endif

	void test_table_shape()
	{
		static_assert(pdn::concepts::constant_generator<pdn::default_constant_generator<u8_t>, u8_t>);
		static_assert(pdn::concepts::constant_generator<pdn::default_constant_generator<u16_t>, u16_t>);
		static_assert(pdn::concepts::constant_generator<pdn::default_constant_generator<u32_t>, u32_t>);

#ifndef PDN_NO_EXTENSIONS
		constexpr ::std::size_t expected_size = 51u;
#else
		constexpr ::std::size_t expected_size = 45u;
#endif
		CHECK(pdn::constant_table<u8_t>::instance().size() == expected_size);
		CHECK(pdn::constant_table<u16_t>::instance().size() == expected_size);
		CHECK(pdn::constant_table<u32_t>::instance().size() == expected_size);
		// one instance per character type, stable across calls
		CHECK(&pdn::constant_table<u8_t>::instance() == &pdn::constant_table<u8_t>::instance());
	}

	void test_names()
	{
		for (const auto& n : standard_names)
		{
			auto v = gen(n.iden);
			CHECK(v.has_value());
			if (v) CHECK(v->type_of() == n.type);
		}
#ifndef PDN_NO_EXTENSIONS
		for (const auto& n : extra_names)
		{
			auto v = gen(n.iden);
			CHECK(v.has_value());
			if (v) CHECK(v->type_of() == n.type);
		}
#else
		// The six non-standard constants must be gone.
		CHECK(!gen(u8"\u03C0").has_value());
		CHECK(!gen(u8"\u03B3").has_value());
		CHECK(!gen(u8"\u03A6").has_value());
		CHECK(!gen(u8"hello").has_value());
		CHECK(!gen(u8"fib_10_list").has_value());
		CHECK(!gen(u8"me_object").has_value());
#endif

		// unknown spellings, including case changes and a trailing space
		CHECK(!gen(u8"zzz").has_value());
		CHECK(!gen(u8"PI").has_value());
		CHECK(!gen(u8"Pi").has_value());
		CHECK(!gen(u8"i8_Min").has_value());
		CHECK(!gen(u8"pi ").has_value());
		CHECK(!gen(u8"").has_value());
		CHECK(!gen(u8"infinit").has_value());
	}

	void test_heterogeneous_lookup()
	{
		// the transparent hasher lets the table be queried with a string_view
		const auto& table = pdn::constant_table<u8_t>::instance();
		u8sv key{ u8"pi" };
		CHECK(table.find(key) != table.end());
		CHECK(table.count(key) == 1u);
		u8sv missing{ u8"nope" };
		CHECK(table.find(missing) == table.end());
		CHECK(::std::hash<u8sv>{}(key) == ::std::hash<u8s>{}(u8s{ key }));
	}

	void test_values()
	{
		CHECK(pdn::as_int(*gen(u8"i8_min")) == -128);
		CHECK(pdn::as_int(*gen(u8"i8_max")) == 127);
		CHECK(pdn::as_int(*gen(u8"i16_min")) == -32768);
		CHECK(pdn::as_int(*gen(u8"i32_min")) == -2147483648ll);
		CHECK(pdn::as_int(*gen(u8"i64_min")) == ::std::numeric_limits<::std::int64_t>::min());
		CHECK(pdn::as_int(*gen(u8"i64_max")) == ::std::numeric_limits<::std::int64_t>::max());
		CHECK(pdn::as_uint(*gen(u8"u8_max")) == 255u);
		CHECK(pdn::as_uint(*gen(u8"u16_max")) == 65535u);
		CHECK(pdn::as_uint(*gen(u8"u32_max")) == 4294967295u);
		CHECK(pdn::as_uint(*gen(u8"u64_max")) == ::std::numeric_limits<::std::uint64_t>::max());
		CHECK(pdn::as_uint(*gen(u8"u64_min")) == 0u);
		CHECK(pdn::as_int(*gen(u8"int_min")) == ::std::numeric_limits<pdn::type::auto_int>::min());
		CHECK(pdn::as_uint(*gen(u8"uint_max")) == ::std::numeric_limits<pdn::type::auto_uint>::max());
		CHECK(pdn::as_bool(*gen(u8"true")));
		CHECK(!pdn::as_bool(*gen(u8"false")));
		CHECK(pdn::as_fp(*gen(u8"pi")) == ::std::numbers::pi);
		CHECK(pdn::as_fp(*gen(u8"e")) == ::std::numbers::e);
		CHECK(pdn::as_fp(*gen(u8"phi")) == ::std::numbers::phi);
#ifndef PDN_NO_EXTENSIONS
		CHECK(pdn::as_fp(*gen(u8"\u03C0")) == ::std::numbers::pi);
		CHECK(pdn::as_fp(*gen(u8"\u03B3")) == ::std::numbers::egamma);
		CHECK(pdn::as_fp(*gen(u8"\u03A6")) == ::std::numbers::phi);
#endif
		CHECK(::std::isinf(pdn::as_fp(*gen(u8"infinity"))));
		CHECK(::std::isinf(pdn::as_fp(*gen(u8"inf"))));
		CHECK(::std::isnan(pdn::as_fp(*gen(u8"quiet_NaN"))));
		CHECK(::std::isnan(pdn::as_fp(*gen(u8"snan"))));
	}

	void test_test_values()
	{
#ifndef PDN_NO_EXTENSIONS
		CHECK(pdn::as_string(*gen(u8"hello")) == u8s{ u8"Hello, world!" });

		auto fib_entity = gen(u8"fib_10_list");
		CHECK(fib_entity.has_value());
		const auto& fib = pdn::as_list(*fib_entity);
		constexpr ::std::int64_t expect[] = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55 };
		CHECK(fib.size() == 10u);
		for (::std::size_t i = 0; i != fib.size() && i != 10u; ++i)
		{
			CHECK(pdn::as_int(fib[i]) == expect[i]);
		}

		auto me_entity = gen(u8"me_object");
		CHECK(me_entity.has_value());
		const auto& me = pdn::as_object(*me_entity);
		auto name_it = me.find(u8sv{ u8"name" });
		CHECK(name_it != me.end());
		if (name_it != me.end()) CHECK(pdn::as_string(name_it->second) == u8s{ u8"PDN" });
		auto date_it = me.find(u8sv{ u8"date" });
		CHECK(date_it != me.end());
		if (date_it != me.end())
		{
			const auto& date = pdn::as_object(date_it->second);
			auto y = date.find(u8sv{ u8"y" });
			auto m = date.find(u8sv{ u8"m" });
			auto d = date.find(u8sv{ u8"d" });
			CHECK(y != date.end() && pdn::as_int(y->second) == 2024);
			CHECK(m != date.end() && pdn::as_int(m->second) == 12);
			CHECK(d != date.end() && pdn::as_int(d->second) == 21);
		}
#else
		// These six constants are absent when PDN_NO_EXTENSIONS is defined (checked by test_names).
#endif
	}

	void test_copies_are_independent()
	{
#ifndef PDN_NO_EXTENSIONS
		// every lookup hands out a copy: proxy-bearing entities deep-copy, so writing to a
		// returned string must not change the table
		auto a = gen(u8"hello");
		auto b = gen(u8"hello");
		CHECK(a.has_value() && b.has_value());
		CHECK(pdn::as_string(*a).data() != pdn::as_string(*b).data());

		const auto& table = pdn::constant_table<u8_t>::instance();
		auto it = table.find(u8s{ u8"hello" });
		CHECK(it != table.end());
		CHECK(pdn::as_string(it->second).data() != pdn::as_string(*a).data());

		a->get<u8s>()[0] = u8'X';
		CHECK(pdn::as_string(*b) == u8s{ u8"Hello, world!" });
		CHECK(pdn::as_string(*gen(u8"hello")) == u8s{ u8"Hello, world!" });
#endif
	}

	void test_other_char_types()
	{
		auto p16 = pdn::constant_generator_std_function<u16_t>(u8s{ u8"pi" });
		CHECK(p16.has_value());
		if (p16) CHECK(p16->type_of() == pdn::type_code::f64);
#ifndef PDN_NO_EXTENSIONS
		// keys stay UTF-8 whatever char_t the value uses
		auto g16 = pdn::constant_generator_std_function<u16_t>(u8s{ u8"\u03C0" });
		CHECK(g16.has_value());
		if (g16) CHECK(pdn::as_fp(*g16) == ::std::numbers::pi);
		auto h32 = pdn::constant_generator_std_function<u32_t>(u8s{ u8"hello" });
		CHECK(h32.has_value());
		if (h32) CHECK(pdn::as_string(*h32) == pdn::unicode::u32string{ U"Hello, world!" });
#else
		// The extension keys are absent for every character width.
		CHECK(!pdn::constant_generator_std_function<u16_t>(u8s{ u8"\u03C0" }).has_value());
		CHECK(!pdn::constant_generator_std_function<u32_t>(u8s{ u8"hello" }).has_value());
#endif
		auto none = pdn::constant_generator_std_function<u16_t>(u8s{ u8"zzz" });
		CHECK(!none.has_value());
	}

	auto value_of(const char8_t* source) -> const e8&
	{
		static e8 storage{};
		auto r = pdn::parse(::std::u8string_view{ source }, pdn::utf8_tag);
		CHECK((bool)r);
		if (r) storage = r.value().as_object().find(u8sv{ u8"x" })->second;
		return storage;
	}

	void test_end_to_end()
	{
		CHECK(value_of(u8"x: @pi").type_of() == pdn::type_code::f64);
		CHECK(pdn::as_fp(value_of(u8"x: @pi")) == ::std::numbers::pi);
		CHECK(value_of(u8"x: @i8_min").type_of() == pdn::type_code::i8);
		CHECK(pdn::as_int(value_of(u8"x: @i8_min")) == -128);
#ifndef PDN_NO_EXTENSIONS
		CHECK(value_of(u8"x: @hello").type_of() == pdn::type_code::string);
		CHECK(pdn::as_string(value_of(u8"x: @hello")) == u8s{ u8"Hello, world!" });
		CHECK(value_of(u8"x: @me_object").type_of() == pdn::type_code::object);
		CHECK(value_of(u8"x: @\u03C0").type_of() == pdn::type_code::f64);
		// the bare character U+221E is lexed as the at-identifier "infinity"
		CHECK(value_of(u8"x: \u221E").type_of() == pdn::type_code::f64);
		CHECK(::std::isinf(pdn::as_fp(value_of(u8"x: \u221E"))));
		CHECK(pdn::as_fp(value_of(u8"x: \u221E")) == pdn::as_fp(*gen(u8"infinity")));
#else
		// The extension spellings must be rejected end to end (value_of would assert, so parse directly).
		CHECK(!pdn::parse(::std::u8string_view{ u8"x: @hello" }, pdn::utf8_tag));
		CHECK(!pdn::parse(::std::u8string_view{ u8"x: @\u03C0" }, pdn::utf8_tag));
		CHECK(!pdn::parse(::std::u8string_view{ u8"x: \u221E" }, pdn::utf8_tag));
#endif
		// unknown constants are reported by the parser
		auto bad = pdn::parse(::std::u8string_view{ u8"x: @zzz" }, pdn::utf8_tag);
		CHECK(!(bool)bad);
	}
}

int main()
{
	test_table_shape();
	test_names();
	test_heterogeneous_lookup();
	test_values();
	test_test_values();
	test_copies_are_independent();
	test_other_char_types();
	test_end_to_end();

	if (fails != 0) std::printf("%d check(s) failed\n", fails);
	else std::printf("all checks passed\n");
	return fails == 0 ? 0 : 1;
}
