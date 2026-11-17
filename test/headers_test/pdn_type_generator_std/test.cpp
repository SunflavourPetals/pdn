// Review test for pdn/include/pdn/pdn_type_generator_std.h
//
// The table must carry exactly the type names and aliases listed by
// docs/pdn-reference.md ("标准规定了如下别名"), and every table must resolve them to the
// same type codes. The u16/u32 tables are derived from the u8 table, so their key sets are
// checked to be the widened u8 keys.
#include <pdn/pdn_type_generator_std.h>
#include <pdn/pdn_type_generator_concept.h>
#include <pdn/spdn.h>

#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace
{
	int failures = 0;

	void check(bool cond, const char* what)
	{
		if (!cond)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	using pdn::type_code;
	using u8_t = pdn::unicode::u8char_t;
	using u16_t = pdn::unicode::u16char_t;
	using u32_t = pdn::unicode::u32char_t;

	struct name_and_code
	{
		const char* name;
		type_code code;
	};

	// One entry per accepted spelling: 28 from the reference plus cppint/cppuint, two
	// non-standard names that this implementation accepts (see the reference note).
	constexpr name_and_code names[] = {
		{ "i8", type_code::i8 },        { "i16", type_code::i16 },
		{ "i32", type_code::i32 },      { "i64", type_code::i64 },
		{ "u8", type_code::u8 },        { "u16", type_code::u16 },
		{ "u32", type_code::u32 },      { "u64", type_code::u64 },
		{ "f32", type_code::f32 },      { "f64", type_code::f64 },
		{ "f", type_code::f32 },        { "float", type_code::f32 },
		{ "double", type_code::f64 },   { "boolean", type_code::boolean },
		{ "bool", type_code::boolean }, { "character", type_code::character },
		{ "char", type_code::character }, { "c", type_code::character },
		{ "string", type_code::string }, { "str", type_code::string },
		{ "s", type_code::string },     { "list", type_code::list },
		{ "object", type_code::object }, { "obj", type_code::object },
		{ "cppint", type_code::i32 },   { "int", type_code::i32 },
		{ "i", type_code::i32 },        { "cppuint", type_code::u32 },
		{ "uint", type_code::u32 },     { "u", type_code::u32 },
	};

	constexpr std::size_t names_count = sizeof(names) / sizeof(names[0]);

	template <typename char_t>
	pdn::type::string<char_t> to_key(const char* name);

	template <>
	pdn::type::string<u8_t> to_key<u8_t>(const char* name)
	{
		pdn::type::string<u8_t> out;
		for (; *name; ++name)
		{
			out.push_back(static_cast<u8_t>(*name));
		}
		return out;
	}

	template <>
	pdn::type::string<u16_t> to_key<u16_t>(const char* name)
	{
		pdn::type::string<u16_t> out;
		for (; *name; ++name)
		{
			out.push_back(static_cast<u16_t>(*name));
		}
		return out;
	}

	template <>
	pdn::type::string<u32_t> to_key<u32_t>(const char* name)
	{
		pdn::type::string<u32_t> out;
		for (; *name; ++name)
		{
			out.push_back(static_cast<u32_t>(*name));
		}
		return out;
	}

	template <typename char_t>
	void check_names()
	{
		for (const auto& entry : names)
		{
			check(pdn::type_generator_function<char_t>(to_key<char_t>(entry.name)) == entry.code, entry.name);
		}
	}

	// Every accepted spelling must be distinct: the table has one key per spelling.
	constexpr bool names_are_distinct()
	{
		for (std::size_t i = 0; i < names_count; ++i)
		{
			for (std::size_t j = i + 1; j < names_count; ++j)
			{
				const char* a = names[i].name;
				const char* b = names[j].name;
				std::size_t k = 0;
				while (a[k] != '\0' && a[k] == b[k]) { ++k; }
				if (a[k] == b[k]) { return false; }
			}
		}
		return true;
	}
}

static_assert(names_are_distinct(), "the probe table lists a spelling twice");
static_assert(names_count == 30, "the reference lists 28 spellings, plus cppint/cppuint");

// The integer aliases must follow C++ int / unsigned int, as the reference requires.
static_assert(std::is_same_v<pdn::type::auto_int, pdn::type::i32>);
static_assert(std::is_same_v<pdn::type::auto_uint, pdn::type::u32>);
static_assert(pdn::detail::cppint_to_type_code_v<pdn::type::i8> == type_code::i8);
static_assert(pdn::detail::cppint_to_type_code_v<pdn::type::i16> == type_code::i16);
static_assert(pdn::detail::cppint_to_type_code_v<pdn::type::i32> == type_code::i32);
static_assert(pdn::detail::cppint_to_type_code_v<pdn::type::i64> == type_code::i64);
static_assert(pdn::detail::cppuint_to_type_code_v<pdn::type::u8> == type_code::u8);
static_assert(pdn::detail::cppuint_to_type_code_v<pdn::type::u16> == type_code::u16);
static_assert(pdn::detail::cppuint_to_type_code_v<pdn::type::u32> == type_code::u32);
static_assert(pdn::detail::cppuint_to_type_code_v<pdn::type::u64> == type_code::u64);
static_assert(pdn::detail::cppint_to_type_code_v<pdn::type::auto_int> == type_code::i32);
static_assert(pdn::detail::cppuint_to_type_code_v<pdn::type::auto_uint> == type_code::u32);

// The std table is what the default generator (and so the default function package) uses.
static_assert(pdn::concepts::type_generator<pdn::default_type_generator<u8_t>, u8_t>);

int main()
{
	// The three tables have one entry per accepted spelling.
	check(pdn::type_table<u8_t>::instance().size() == names_count, "u8 table size");
	check(pdn::type_table<u16_t>::instance().size() == names_count, "u16 table size");
	check(pdn::type_table<u32_t>::instance().size() == names_count, "u32 table size");

	check_names<u8_t>();
	check_names<u16_t>();
	check_names<u32_t>();

	// instance() is a function-local static: repeated calls must return the same table.
	check(std::addressof(pdn::type_table<u8_t>::instance()) == std::addressof(pdn::type_table<u8_t>::instance()),
	      "u8 instance() is not stable");

	// Unknown spellings are answered with unknown, not with an error.
	check(pdn::type_generator_function<u8_t>(to_key<u8_t>("I8")) == type_code::unknown, "I8");
	check(pdn::type_generator_function<u8_t>(to_key<u8_t>("int8")) == type_code::unknown, "int8");
	check(pdn::type_generator_function<u8_t>(to_key<u8_t>("i128")) == type_code::unknown, "i128");
	check(pdn::type_generator_function<u8_t>(to_key<u8_t>("i 8")) == type_code::unknown, "i 8");
	check(pdn::type_generator_function<u8_t>(pdn::type::string<u8_t>{}) == type_code::unknown, "empty");
	check(pdn::type_generator_function<u8_t>(pdn::type::string<u8_t>{ u8"\u00E9" }) == type_code::unknown,
	      "non-ascii u8");

	// The derived tables hold no key that the u8 table does not hold.
	for (const auto& entry : names)
	{
		const auto key16 = to_key<u16_t>(entry.name);
		check(pdn::type_table<u16_t>::instance().contains(key16), "u16 key derived from u8");
		const auto key32 = to_key<u32_t>(entry.name);
		check(pdn::type_table<u32_t>::instance().contains(key32), "u32 key derived from u8");
	}
	check(!pdn::type_table<u16_t>::instance().contains(pdn::type::string<u16_t>{ u"\u00E9" }),
	      "u16 table must not hold a non-ascii key");

	// End to end: the names reach the parser through default_type_generator.
	{
		const auto r = pdn::parse(std::u8string_view{ u8"a: cppint 1\nb: int 2\nc: f 1.5\nd: obj {x 1}\n" }, pdn::utf8_tag);
		check(r.success(), "parse with aliases");
		if (r.success())
		{
			const auto text = pdn::make_u8serializer().serialize(pdn::as_object(r.value()));
			const std::u8string_view sv = text;
			// int/i32 is the "auto" integer type, so the serializer omits that type; f32 is not
			// auto, so it must be written explicitly.
			check(sv.find(u8"c: f32 1.5;") != std::u8string_view::npos, "f32 type is written");
			check(sv.find(u8"b: 2;") != std::u8string_view::npos, "auto int type is omitted");
			check(sv.find(u8"a: 1;") != std::u8string_view::npos, "cppint type is omitted like int");
			check(sv.find(u8"d: {") != std::u8string_view::npos, "obj alias");
		}
	}

	// A non-spec name is reported as unknown_type by the parser.
	{
		const auto r = pdn::parse(std::u8string_view{ u8"x: i128 1\n" }, pdn::utf8_tag);
		check(!r.success(), "i128 must not parse");
	}

	if (failures != 0)
	{
		std::printf("%d check(s) failed\n", failures);
		return 1;
	}
	std::printf("probe ok\n");
	return 0;
}
