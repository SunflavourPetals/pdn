#include <pdn/pdn_utf8_decoder.h>
#include <pdn/pdn_utf8_encoder.h>

#include <array>
#include <cstdio>

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
using namespace pdn::unicode::utf8;

namespace
{
	void test_size_and_bytes()
	{
		static_assert(encode(0x41u).size() == 1);
		static_assert(encode(0x80u).size() == 2);
		static_assert(encode(0x4E2Du).size() == 3);
		static_assert(encode(0x1F600u).size() == 4);
		static_assert(encode(0x10FFFFu).size() == 4);

		auto a = encode(0x41u);
		CHECK(a.begin()[0] == char8_t(0x41));
		CHECK(a.end() - a.begin() == 1);

		auto b = encode(0x4E2Du);
		CHECK(b.begin()[0] == char8_t(0xE4));
		CHECK(b.begin()[1] == char8_t(0xB8));
		CHECK(b.begin()[2] == char8_t(0xAD));

		auto c = encode(0x1F600u);
		CHECK(c.begin()[0] == char8_t(0xF0));
		CHECK(c.begin()[1] == char8_t(0x9F));
		CHECK(c.begin()[2] == char8_t(0x98));
		CHECK(c.begin()[3] == char8_t(0x80));
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
		std::array<code_point_t, 12> const samples{
			0x0000u, 0x007Fu, 0x0080u, 0x07FFu, 0x0800u, 0x4E2Du,
			0xFFFFu, 0x10000u, 0x1F600u, 0x10FFFFu, 0x00E9u, 0x2028u };

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
	test_size_and_bytes();
	test_non_scalar();
	test_round_trip();

	if (failures == 0) { std::printf("pdn_utf8_encoder: ok\n"); }
	return failures == 0 ? 0 : 1;
}