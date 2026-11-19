#include <concepts>
#include <cstdio>
#include <string_view>

#include <pdn/pdn_utf_decoder.h>

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
	// the alias dispatches on the character type
	static_assert(std::same_as<decoder<char8_t>,  utf8::decoder>);
	static_assert(std::same_as<decoder<char16_t>, utf16::decoder>);
	static_assert(std::same_as<decoder<char32_t>, utf32::decoder>);

	// the iterator concepts dispatch on the code unit type
	static_assert(detail::utf8_iterator<const char8_t*>);
	static_assert(!detail::utf8_iterator<const char*>);
	static_assert(!detail::utf8_iterator<const char16_t*>);
	static_assert(!detail::utf8_iterator<const char32_t*>);
	static_assert(detail::utf16_iterator<const char16_t*>);
	static_assert(!detail::utf16_iterator<const char8_t*>);
	static_assert(detail::utf32_iterator<const char32_t*>);
	static_assert(!detail::utf32_iterator<const char16_t*>);

	struct frozen
	{
		char8_t operator*() const;
	};
	static_assert(!detail::utf8_iterator<frozen>); // no operator++

	void test_utf8_stop_position()
	{
		const char8_t buf[] = u8"A\u4E2D\U0001F600"; // 1 + 3 + 4 code units
		const char8_t* p = buf;

		auto r = decode<false>(p, buf + 8);
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'A');
		CHECK(p == buf); // one unit long: the only unit is the last unit
		++p;

		r = decode<false>(p, buf + 8);
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'\u4E2D');
		CHECK(p == buf + 3);
		++p;

		r = decode<false>(p, buf + 8);
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'\U0001F600');
		CHECK(p == buf + 7);
		++p;

		CHECK(p == buf + 8);
		CHECK(decode<false>(p, buf + 8).errc() == utf8::decode_error_code::unexpected_eof_offset0);
	}

	void test_utf8_reach_next()
	{
		const char8_t buf[] = u8"A\u4E2D";
		const char8_t* p = buf;

		auto r = decode<true>(p, buf + 4);
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'A');
		CHECK(p == buf + 1);

		r = decode<true>(p, buf + 4);
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'\u4E2D');
		CHECK(p == buf + 4);
	}

	void test_utf16_surrogate_pair()
	{
		const char16_t buf[] = { u'A', static_cast<char16_t>(0xD83D), static_cast<char16_t>(0xDE00) };
		const char16_t* p = buf;

		auto r = decode<false>(p, buf + 3);
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'A');
		CHECK(p == buf);
		++p;

		r = decode<false>(p, buf + 3);
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'\U0001F600');
		CHECK(p == buf + 2);
		++p;

		CHECK(p == buf + 3);
	}

	void test_utf32()
	{
		const char32_t buf[] = { U'A', U'\U0001F600' };
		const char32_t* p = buf;

		auto r = decode<true>(p, buf + 2);
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'A');
		CHECK(p == buf + 1);

		r = decode<true>(p, buf + 2);
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'\U0001F600');
		CHECK(p == buf + 2);
	}

	void test_error_forwarding()
	{
		const char8_t lone[] = { static_cast<char8_t>(0x80) };
		const char8_t* p = lone;
		auto r = decode<false>(p, lone + 1);
		CHECK(!static_cast<bool>(r));
		CHECK(r.errc() == utf8::decode_error_code::trailing_as_start);
		CHECK(!utf8::decoder::is_reaching_next<false>(r));

		// overlong: the whole sequence is consumed, so the caller must not advance again
		const char8_t overlong[] = { static_cast<char8_t>(0xE0), static_cast<char8_t>(0x80), static_cast<char8_t>(0x80) };
		p = overlong;
		r = decode<false>(p, overlong + 3);
		CHECK(!static_cast<bool>(r));
		CHECK(r.errc() == utf8::decode_error_code::non_shortest_sequence);
		CHECK(!utf8::decoder::is_reaching_next<false>(r));

		// cut in the middle of a sequence: the offset counts the code units already consumed
		const char8_t cut[] = { static_cast<char8_t>(0xE4), static_cast<char8_t>(0xB8) };
		p = cut;
		r = decode<false>(p, cut + 2);
		CHECK(!static_cast<bool>(r));
		CHECK(r.errc() == utf8::decode_error_code::unexpected_eof_offset2);
		CHECK(utf8::decoder::is_reaching_next<false>(r));
	}

	void test_string_view()
	{
		const std::u8string_view sv = u8"\u4E2D";
		const char8_t* p = sv.cbegin();
		auto r = decode<true>(p, sv.cend());
		CHECK(static_cast<bool>(r));
		CHECK(r.value() == U'\u4E2D');
		CHECK(p == sv.cend());
	}
}

int main()
{
	test_utf8_stop_position();
	test_utf8_reach_next();
	test_utf16_surrogate_pair();
	test_utf32();
	test_error_forwarding();
	test_string_view();

	if (failures == 0) { std::printf("all utf-decoder facade checks passed\n"); }
	return failures == 0 ? 0 : 1;
}
