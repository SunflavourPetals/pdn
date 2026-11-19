// Tests for pdn/include/pdn/pdn_type_to_type_code.h.
//
// What is pinned down here:
//   * every C++ type entity stores maps back to its PDN type code, and the two mapping
//     headers round-trip in both directions;
//   * the mapping key is exact: cv-qualified and reference forms are unknown, and for the
//     character/string/list/object families char_t is part of the key;
//   * the primary template answers unknown, which is also what void maps to;
//   * type_of() uses this mapping: values parsed from PDN source report the type that was
//     written, and an empty reference reports unknown.

#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>

#include <pdn/pdn_type_to_type_code.h>
#include <pdn/pdn_type_code_to_type.h>
#include <pdn/spdn.h>

namespace
{
	using pdn::type_code;
	using pdn::type_code_to_type_t;
	using pdn::type_to_type_code_v;
	using u8c = pdn::unicode::u8char_t;
	using u16c = pdn::unicode::u16char_t;
	using u32c = pdn::unicode::u32char_t;

	int failures = 0;

	void check(bool ok, const char* what)
	{
		if (!ok)
		{
			std::printf("FAILED: %s\n", what);
			++failures;
		}
	}

	// Scalars: exact alias in, code out, independent of char_t.
	static_assert(type_to_type_code_v<pdn::type::i8, u8c> == type_code::i8);
	static_assert(type_to_type_code_v<pdn::type::i16, u8c> == type_code::i16);
	static_assert(type_to_type_code_v<pdn::type::i32, u8c> == type_code::i32);
	static_assert(type_to_type_code_v<pdn::type::i64, u8c> == type_code::i64);
	static_assert(type_to_type_code_v<pdn::type::u8, u8c> == type_code::u8);
	static_assert(type_to_type_code_v<pdn::type::u16, u8c> == type_code::u16);
	static_assert(type_to_type_code_v<pdn::type::u32, u8c> == type_code::u32);
	static_assert(type_to_type_code_v<pdn::type::u64, u8c> == type_code::u64);
	static_assert(type_to_type_code_v<pdn::type::f32, u8c> == type_code::f32);
	static_assert(type_to_type_code_v<pdn::type::f64, u8c> == type_code::f64);
	static_assert(type_to_type_code_v<pdn::type::boolean, u8c> == type_code::boolean);
	static_assert(type_to_type_code_v<pdn::type::u64, u32c> == type_code::u64);

	// Composites: char_t is part of the key.
	static_assert(type_to_type_code_v<pdn::type::character<u8c>, u8c> == type_code::character);
	static_assert(type_to_type_code_v<pdn::type::character<u16c>, u16c> == type_code::character);
	static_assert(type_to_type_code_v<pdn::type::character<u32c>, u32c> == type_code::character);
	static_assert(type_to_type_code_v<pdn::type::string<u8c>, u8c> == type_code::string);
	static_assert(type_to_type_code_v<pdn::type::string<u16c>, u16c> == type_code::string);
	static_assert(type_to_type_code_v<pdn::type::string<u32c>, u32c> == type_code::string);
	static_assert(type_to_type_code_v<pdn::type::list<u8c>, u8c> == type_code::list);
	static_assert(type_to_type_code_v<pdn::type::list<u16c>, u16c> == type_code::list);
	static_assert(type_to_type_code_v<pdn::type::object<u8c>, u8c> == type_code::object);
	static_assert(type_to_type_code_v<pdn::type::object<u32c>, u32c> == type_code::object);

	// The aliases from pdn_type.h agree with the generic parameters.
	static_assert(type_to_type_code_v<pdn::type::u8char, u8c> == type_code::character);
	static_assert(type_to_type_code_v<pdn::type::u16string, u16c> == type_code::string);
	static_assert(type_to_type_code_v<pdn::type::u32list, u32c> == type_code::list);
	static_assert(type_to_type_code_v<pdn::type::u8object, u8c> == type_code::object);

	// The key is exact: a char_t mismatch does not match.
	static_assert(type_to_type_code_v<pdn::type::u8string, u16c> == type_code::unknown);
	static_assert(type_to_type_code_v<pdn::type::character<u8c>, u32c> == type_code::unknown);

	// cv-qualified and reference forms do not match; the primary template answers unknown.
	static_assert(type_to_type_code_v<const pdn::type::i8, u8c> == type_code::unknown);
	static_assert(type_to_type_code_v<volatile pdn::type::u8, u8c> == type_code::unknown);
	static_assert(type_to_type_code_v<pdn::type::i8&, u8c> == type_code::unknown);
	static_assert(type_to_type_code_v<pdn::type::i8&&, u8c> == type_code::unknown);
	static_assert(type_to_type_code_v<const pdn::type::u8string&, u8c> == type_code::unknown);

	// Types PDN has no notion of, and void, are unknown.
	static_assert(type_to_type_code_v<void, u8c> == type_code::unknown);
	static_assert(type_to_type_code_v<int[3], u8c> == type_code::unknown);
	struct user_type {};
	static_assert(type_to_type_code_v<user_type, u8c> == type_code::unknown);

	// int is the fixed-width i32 on this platform, so it maps through that alias.
	static_assert(std::is_same_v<pdn::type::i32, int>);
	static_assert(type_to_type_code_v<int, u8c> == type_code::i32);

	// The struct form exposes the same value.
	static_assert(pdn::type_to_type_code<pdn::type::boolean, u8c>::value == type_code::boolean);

	// Both mapping headers round-trip on the fifteen real codes.
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::i8, u8c>, u8c> == type_code::i8);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::i16, u8c>, u8c> == type_code::i16);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::i32, u8c>, u8c> == type_code::i32);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::i64, u8c>, u8c> == type_code::i64);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::u8, u8c>, u8c> == type_code::u8);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::u16, u8c>, u8c> == type_code::u16);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::u32, u8c>, u8c> == type_code::u32);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::u64, u8c>, u8c> == type_code::u64);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::f32, u8c>, u8c> == type_code::f32);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::f64, u8c>, u8c> == type_code::f64);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::boolean, u8c>, u8c> == type_code::boolean);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::character, u8c>, u8c> == type_code::character);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::string, u8c>, u8c> == type_code::string);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::list, u8c>, u8c> == type_code::list);
	static_assert(type_to_type_code_v<type_code_to_type_t<type_code::object, u8c>, u8c> == type_code::object);
	static_assert(std::is_void_v<type_code_to_type_t<type_to_type_code_v<void, u8c>, u8c>>);

	void test_type_of()
	{
		const auto result = pdn::parse(::std::u8string_view{
			u8"a: i8 1\n"
			u8"b: u64 2\n"
			u8"c: f64 1.5\n"
			u8"d: boolean 1\n"
			u8"e: character 'x'\n"
			u8"f: string \"s\"\n"
			u8"g: list [1, 2]\n"
			u8"h: object {x 1}\n"
		}, pdn::utf8_tag);

		check(result.success(), "the sample source parses");
		if (!result.success())
		{
			return;
		}

		using namespace std::string_view_literals;
		const auto& obj = result.value();
		check(pdn::type_of(obj[u8"a"sv]) == type_code::i8, "i8 value reports i8");
		check(pdn::type_of(obj[u8"b"sv]) == type_code::u64, "u64 value reports u64");
		check(pdn::type_of(obj[u8"c"sv]) == type_code::f64, "f64 value reports f64");
		check(pdn::type_of(obj[u8"d"sv]) == type_code::boolean, "boolean value reports boolean");
		check(pdn::type_of(obj[u8"e"sv]) == type_code::character, "character value reports character");
		check(pdn::type_of(obj[u8"f"sv]) == type_code::string, "string value reports string (through the proxy)");
		check(pdn::type_of(obj[u8"g"sv]) == type_code::list, "list value reports list (through the proxy)");
		check(pdn::type_of(obj[u8"h"sv]) == type_code::object, "object value reports object (through the proxy)");
		check(pdn::type_of(pdn::const_refer<u8c>{}) == type_code::unknown, "an empty reference reports unknown");
	}
}

int main()
{
	test_type_of();

	if (failures != 0)
	{
		std::printf("%d check(s) failed\n", failures);
		return 1;
	}

	std::printf("all checks passed\n");
	return 0;
}
