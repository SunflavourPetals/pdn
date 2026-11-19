#include <concepts>
#include <cstddef>
#include <cstdio>

#include <pdn/pdn_utf_encoder.h>

namespace
{
	int failures = 0;

	void check(bool ok, const char* what, int line)
	{
		if (!ok)
		{
			++failures;
			std::printf("FAILED: %s (line %d)\n", what, line);
		}
	}
}

#define CHECK(...) check((__VA_ARGS__), #__VA_ARGS__, __LINE__)

using namespace pdn::unicode;

namespace
{
	// the alias and the result alias dispatch on the character type
	static_assert(std::same_as<encoder<char8_t>,  utf8::encoder>);
	static_assert(std::same_as<encoder<char16_t>, utf16::encoder>);
	static_assert(std::same_as<encoder<char32_t>, utf32::encoder>);
	static_assert(std::same_as<detail::encode_result_t<char8_t>,  utf8::encode_result>);
	static_assert(std::same_as<detail::encode_result_t<char16_t>, utf16::encode_result>);
	static_assert(std::same_as<detail::encode_result_t<char32_t>, utf32::encode_result>);

	// encode<char_t> is constrained to the three code unit types
	template <typename char_t>
	concept encodable = requires { pdn::unicode::encode<char_t>(U'A'); };
	static_assert(encodable<char8_t>);
	static_assert(encodable<char16_t>);
	static_assert(encodable<char32_t>);
	static_assert(!encodable<int>);
	static_assert(!encodable<char>);
	static_assert(!encodable<wchar_t>);

	void test_encode_dispatch()
	{
		CHECK(encode<char8_t>(U'A').size() == 1);
		CHECK(encode<char16_t>(U'A').size() == 1);
		CHECK(encode<char32_t>(U'A').size() == 1);

		CHECK(encode<char8_t>(U'\u4E2D').size() == 3);
		CHECK(encode<char16_t>(U'\u4E2D').size() == 1);
		CHECK(encode<char32_t>(U'\u4E2D').size() == 1);

		CHECK(encode<char8_t>(U'\U0001F600').size() == 4);
		CHECK(encode<char16_t>(U'\U0001F600').size() == 2);
		CHECK(encode<char32_t>(U'\U0001F600').size() == 1);

		const char8_t expect[] = { static_cast<char8_t>(0xE4), static_cast<char8_t>(0xB8), static_cast<char8_t>(0xAD) };
		auto r = encode<char8_t>(U'\u4E2D');
		CHECK(static_cast<bool>(r));
		CHECK(r.begin()[0] == expect[0] && r.begin()[1] == expect[1] && r.begin()[2] == expect[2]);

		auto r32 = encode<char32_t>(U'\u4E2D');
		CHECK(static_cast<bool>(r32));
		CHECK(r32.begin()[0] == static_cast<char32_t>(0x4E2D));

		auto r16 = encode<char16_t>(U'\U0001F600');
		CHECK(r16.begin()[0] == static_cast<char16_t>(0xD83D));
		CHECK(r16.begin()[1] == static_cast<char16_t>(0xDE00));
	}

	void test_spellings_agree()
	{
		for (const code_point_t cp : { code_point_t{ U'A' }, code_point_t{ U'\u4E2D' }, code_point_t{ U'\U0001F600' }, code_point_t{ U'\U0010FFFF' } })
		{
			auto a = encode<char8_t>(cp);
			auto b = encode(cp, u8char_t{});
			auto c = utf8::encode(cp);
			auto d = encoder<char8_t>::encode(cp);
			CHECK(a.size() == b.size() && a.size() == c.size() && a.size() == d.size());

			bool same = true;
			for (std::size_t i = 0; i < a.size(); ++i)
			{
				same = same && a.begin()[i] == b.begin()[i];
				same = same && a.begin()[i] == c.begin()[i];
				same = same && a.begin()[i] == d.begin()[i];
			}
			CHECK(same);
		}
	}

	void test_tag_selects_by_type_only()
	{
		// the second parameter is a tag: its value is never read, only its type selects the codec
		auto r16 = encode(U'\U0001F600', u16char_t{ static_cast<char16_t>(0xFFFF) });
		CHECK(static_cast<bool>(r16));
		CHECK(r16.size() == 2);

		auto r32 = encode(U'\U0001F600', code_point_t{ 0xFFFF }); // code_point_t == char32_t
		static_assert(std::same_as<decltype(r32), utf32::encode_result>);
		CHECK(static_cast<bool>(r32));
		CHECK(r32.size() == 1);
	}

	void test_invalid_code_points()
	{
		const code_point_t bad[] = {
			code_point_t{ 0xD800 }, code_point_t{ 0xDFFF }, code_point_t{ 0x110000 }, code_point_t{ 0xFFFFFFFF }
		};
		for (const code_point_t cp : bad)
		{
			CHECK(!static_cast<bool>(encode<char8_t>(cp)));
			CHECK(!static_cast<bool>(encode<char16_t>(cp)));
			CHECK(!static_cast<bool>(encode<char32_t>(cp)));
			CHECK(!static_cast<bool>(encode(cp, u8char_t{})));
			CHECK(!static_cast<bool>(encode(cp, u16char_t{})));
			CHECK(!static_cast<bool>(encode(cp, u32char_t{})));
			CHECK(!static_cast<bool>(encoder<char8_t>::encode(cp)));
		}
	}
}

int main()
{
	test_encode_dispatch();
	test_spellings_agree();
	test_tag_selects_by_type_only();
	test_invalid_code_points();

	if (failures == 0) { std::printf("all utf-encoder facade checks passed\n"); }
	return failures == 0 ? 0 : 1;
}
