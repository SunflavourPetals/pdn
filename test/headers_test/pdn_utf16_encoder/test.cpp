#include <array>
#include <cstdio>

#include <pdn/pdn_utf16_decoder.h>
#include <pdn/pdn_utf16_encoder.h>

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
using namespace pdn::unicode::utf16;

namespace
{
	void test_bmp()
	{
		static_assert(encode(0x41u).size() == 1);
		static_assert(encode(0xFFFFu).size() == 1);

		auto a = encode(0x41u);
		CHECK(a.begin()[0] == char16_t(0x41));
		CHECK(a.end() - a.begin() == 1);
	}

	void test_pair()
	{
		static_assert(encode(0x10000u).size() == 2);
		static_assert(encode(0x10FFFFu).size() == 2);

		auto min = encode(0x10000u);
		CHECK(min.begin()[0] == char16_t(0xD800));
		CHECK(min.begin()[1] == char16_t(0xDC00));

		auto max = encode(0x10FFFFu);
		CHECK(max.begin()[0] == char16_t(0xDBFF));
		CHECK(max.begin()[1] == char16_t(0xDFFF));

		auto emoji = encode(0x1F600u);
		CHECK(emoji.begin()[0] == char16_t(0xD83D));
		CHECK(emoji.begin()[1] == char16_t(0xDE00));
		CHECK(emoji.end() - emoji.begin() == 2);
	}

	void test_non_scalar()
	{
		for (code_point_t cp : { code_point_t(0xD800), code_point_t(0xDFFF), code_point_t(0x110000), code_point_t(0xFFFFFFFF) })
		{
			auto r = encode(cp);
			CHECK(r.failed());
			CHECK(r.error() == encode_error_code::not_scalar_value);
			CHECK(r.size() == 0);
			CHECK(!r);
		}
	}

	void test_round_trip()
	{
		std::array<code_point_t, 10> const samples{
			0x0000u, 0x007Fu, 0x0080u, 0xD7FFu, 0xE000u, 0xFFFDu,
			0x10000u, 0x1F600u, 0x10FFFFu, 0x2028u };

		for (code_point_t cp : samples)
		{
			auto encoded = encode(cp);
			CHECK(!encoded.failed());

			auto begin = encoded.begin();
			auto decoded = decoder::decode<true>(begin, encoded.end());
			CHECK(!decoded.failed());
			CHECK(decoded.value() == cp);
			CHECK(begin == encoded.end());
		}
	}
}

int main()
{
	test_bmp();
	test_pair();
	test_non_scalar();
	test_round_trip();

	if (failures == 0) { std::printf("pdn_utf16_encoder: ok\n"); }
	return failures == 0 ? 0 : 1;
}