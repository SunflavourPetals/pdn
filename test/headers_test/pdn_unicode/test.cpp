// Review test for pdn_unicode.h.
// It is a pure umbrella header: its only job is to pull in pdn_unicode_base.h and the two
// codec facades, which in turn pull in the six per-encoding headers. Nothing else in this
// test includes those headers, and every name used below comes from one of them, so if an
// include is dropped from the umbrella this file stops compiling.

#include <pdn/pdn_unicode.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <type_traits>

namespace
{
	int failures = 0;

	template <typename t, typename u>
	constexpr bool same = ::std::is_same_v<t, u>;

	using namespace pdn::unicode_literals;

	void check(bool ok, const char* what, int line)
	{
		if (!ok)
		{
			::std::printf("FAILED (line %d): %s\n", line, what);
			++failures;
		}
	}

#define CHECK(expr) check((expr), #expr, __LINE__)
}

int main()
{
	// ------------------------------------------------- pdn_unicode_base.h (aliases and tables)
	static_assert(same<pdn::unicode::ucpstring, ::std::u32string>);
	static_assert(same<pdn::unicode::u8string_view, ::std::u8string_view>);
	static_assert(pdn::unicode::concepts::code_unit<char8_t>);
	static_assert(pdn::unicode::concepts::code_unit<char16_t>);
	static_assert(pdn::unicode::concepts::code_unit<char32_t>);
	static_assert(!pdn::unicode::concepts::code_unit<int>);
	static_assert(pdn::unicode::bom::utf8.size() == 3);
	static_assert(pdn::unicode::bom::utf32_be.size() == 4);
	static_assert(static_cast<int>(pdn::unicode::to_byte_size(pdn::unicode::bom_type::utf32_be)) == 4);
	static_assert(static_cast<int>(pdn::unicode::to_byte_size(pdn::unicode::bom_type::no_bom)) == 0);
	CHECK(pdn::unicode::bom::utf8[0] == 0xEF);
	CHECK(pdn::unicode::bom::utf32_be[0] == 0x00);
	CHECK(pdn::unicode::to_encode_type(pdn::unicode::bom_type::utf16_le) == pdn::unicode::encode_type::utf16_le);
	CHECK(pdn::unicode::to_encode_type(pdn::unicode::bom_type::no_bom) == pdn::unicode::encode_type::utf8);
	CHECK(pdn::unicode::get_replace<char8_t>().size() == 3);
	CHECK(static_cast<unsigned char>(pdn::unicode::get_replace<char8_t>()[0]) == 0xEF);
	CHECK(u8"abc"_sv.size() == 3); // the literal from the base header is usable here

	// ------------------------------------------------- the two facades and their aliases
	static_assert(same<pdn::unicode::decoder<char8_t>, pdn::unicode::utf8::decoder>);
	static_assert(same<pdn::unicode::decoder<char16_t>, pdn::unicode::utf16::decoder>);
	static_assert(same<pdn::unicode::decoder<char32_t>, pdn::unicode::utf32::decoder>);
	static_assert(same<pdn::unicode::encoder<char8_t>, pdn::unicode::utf8::encoder>);
	static_assert(same<pdn::unicode::encoder<char16_t>, pdn::unicode::utf16::encoder>);
	static_assert(same<pdn::unicode::encoder<char32_t>, pdn::unicode::utf32::encoder>);

	// decode through the facade (dispatches on the iterator's value type)
	{
		const ::std::u8string_view text = u8"\u4E2D"; // U+4E2D
		auto result = pdn::unicode::decode<true>(text.begin(), text.end());
		static_assert(same<decltype(result), pdn::unicode::utf8::decode_result>);
		CHECK(!result.failed());
		CHECK(result.value() == 0x4E2D);
	}
	// encode through the facade (dispatches on the target char type)
	{
		auto utf8_result = pdn::unicode::encode<char8_t>(0x1F600);
		CHECK(utf8_result.size() == 4);
		CHECK(static_cast<unsigned char>(*utf8_result.begin()) == 0xF0);
		auto utf16_result = pdn::unicode::encode<char16_t>(0x1F600);
		CHECK(utf16_result.size() == 2); // surrogate pair
		CHECK(*utf16_result.begin() == 0xD83D);
		auto utf32_result = pdn::unicode::encode<char32_t>(0x1F600);
		CHECK(utf32_result.size() == 1);
		CHECK(*utf32_result.begin() == 0x1F600);
	}

	// ------------------------------------------------- the six per-encoding headers are here
	{
		auto r8 = pdn::unicode::utf8::encode(U'\u4E2D');
		CHECK(r8.size() == 3);
		auto r16 = pdn::unicode::utf16::encode(U'\u4E2D');
		CHECK(r16.size() == 1);
		CHECK(*r16.begin() == 0x4E2D);
		auto r32 = pdn::unicode::utf32::encode(U'\u4E2D');
		CHECK(r32.size() == 1);
		CHECK(*r32.begin() == 0x4E2D);
	}
	{
		const char8_t bytes8[] = { static_cast<char8_t>(0xE4), static_cast<char8_t>(0xB8), static_cast<char8_t>(0xAD) };
		auto d8 = pdn::unicode::utf8::decode<true>(bytes8 + 0, bytes8 + 3);
		CHECK(!d8.failed());
		CHECK(d8.value() == 0x4E2D);
	}
	{
		const char16_t units16[] = { 0xD83D, 0xDE00 }; // U+1F600 as a surrogate pair
		auto d16 = pdn::unicode::utf16::decode<true>(units16 + 0, units16 + 2);
		CHECK(!d16.failed());
		CHECK(d16.value() == 0x1F600);
	}
	{
		const char32_t units32[] = { 0x1F600 };
		auto d32 = pdn::unicode::utf32::decode<true>(units32 + 0, units32 + 1);
		CHECK(!d32.failed());
		CHECK(d32.value() == 0x1F600);
	}

	if (failures != 0)
	{
		::std::printf("%d check(s) failed\n", failures);
		return 1;
	}
	::std::printf("all unicode umbrella checks passed\n");
	return 0;
}
