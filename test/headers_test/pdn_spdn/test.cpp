// Checks that <pdn/spdn.h> alone is enough for the documented workflow: parse spdn text,
// inspect the entity, serialize it back. The umbrella is intentionally the only pdn header
// included here, so a missing include shows up as a compile error rather than a happy path.

#include <concepts>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

#include <pdn/spdn.h>

namespace
{
	using u8char_t = pdn::unicode::u8char_t;

	::std::size_t failures = 0;

	void check(const bool condition, const char* what)
	{
		if (!condition)
		{
			++failures;
			::std::cout << "FAILED: " << what << '\n';
		}
	}

	// the three documented pieces are visible through the umbrella alone
	static_assert(::std::same_as<pdn::entity<u8char_t>, pdn::entity<u8char_t>>);
	static_assert(::std::same_as<decltype(pdn::make_u8serializer()), pdn::serializer<char8_t>>);
	static_assert(::std::same_as<decltype(pdn::parse(::std::u8string_view{ u8"" }, pdn::utf8_tag)), pdn::parse_result<u8char_t>>);

	void test_parse_then_serialize()
	{
		const auto parsed = pdn::parse(::std::u8string_view{ u8"a: 1; b: [2, 3]" }, pdn::utf8_tag);
		check(parsed.error_code() == pdn::parse_error_code{}, "parse succeeds");
		if (!parsed)
		{
			return;
		}

		const auto& object = parsed.value().as_object();
		check(object.size() == 2, "two entities");
		check(parsed.value().as_object().at(u8"a").as_int() == 1, "a is 1");
		check(parsed.value().as_object().at(u8"b").as_list().size() == 2, "b has two elements");

		const auto text = pdn::make_u8serializer().serialize(parsed.value().as_object());
		check(!text.empty(), "serialize produces text");
		check(text.find(u8"a") != ::std::u8string::npos, "serialized text keeps a");
		check(text.find(u8"b") != ::std::u8string::npos, "serialized text keeps b");

		const auto again = pdn::parse(::std::u8string_view{ text }, pdn::utf8_tag);
		check(again.error_code() == pdn::parse_error_code{}, "reparse of the serialized text succeeds");
		check(again ? again.value().as_object().at(u8"b").as_list().size() == 2 : false, "reparse keeps b");
	}

	void test_round_trip()
	{
		const auto first = pdn::parse(::std::u8string_view{ u8"x: 1.5; y: 'c'; z: \"s\"" }, pdn::utf8_tag);
		check(first.error_code() == pdn::parse_error_code{}, "first parse succeeds");
		if (!first)
		{
			return;
		}
		const auto text = pdn::make_u8serializer().serialize(first.value().as_object());

		const auto second = pdn::parse(::std::u8string_view{ text }, pdn::utf8_tag);
		check(second.error_code() == pdn::parse_error_code{}, "second parse succeeds");
		check(second ? second.value().as_object().size() == 3 : false, "round trip keeps three entities");
		check(second ? second.value().as_object().at(u8"x").as_fp() == 1.5 : false, "round trip keeps x");
	}

	void test_unicode_widths()
	{
		const auto parsed16 = pdn::parse(::std::u16string_view{ u"a: 1" }, pdn::utf16_tag);
		check(parsed16.error_code() == pdn::parse_error_code{}, "utf16 parse succeeds");
		const auto text16 = pdn::make_u16serializer().serialize(parsed16.value().as_object());
		check(!text16.empty(), "utf16 serialize produces text");

		const auto parsed32 = pdn::parse(::std::u32string_view{ U"a: 1" }, pdn::utf32_tag);
		check(parsed32.error_code() == pdn::parse_error_code{}, "utf32 parse succeeds");
		const auto text32 = pdn::make_u32serializer().serialize(parsed32.value().as_object());
		check(!text32.empty(), "utf32 serialize produces text");
	}
}

int main()
{
	test_parse_then_serialize();
	test_round_trip();
	test_unicode_widths();

	if (failures == 0)
	{
		::std::cout << "all spdn umbrella checks passed\n";
		return 0;
	}
	::std::cout << failures << " check(s) failed\n";
	return 1;
}
