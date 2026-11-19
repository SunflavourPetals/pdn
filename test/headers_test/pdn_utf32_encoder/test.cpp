#include <array>
#include <cstdio>

#include <pdn/pdn_utf32_decoder.h>
#include <pdn/pdn_utf32_encoder.h>

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
using namespace pdn::unicode::utf32;

namespace
{
	void test_one_unit()
	{
		static_assert(encode(0x0u).size() == 1);
		static_assert(encode(0xFFFFu).size() == 1);
		static_assert(encode(0x10FFFFu).size() == 1);

		for (code_point_t cp : { code_point_t(0x0), code_point_t(0x41), code_point_t(0xFFFF), code_point_t(0x10000), code_point_t(0x10FFFF) })
		{
			auto r = encode(cp);
			CHECK(!r.failed());
			CHECK(r.size() == 1);
			CHECK(r.begin()[0] == cp);
			CHECK(r.end() - r.begin() == 1);
		}
	}

	void test_non_scalar()
	{
		for (code_point_t cp : { code_point_t(0xD800), code_point_t(0xDBFF), code_point_t(0xDC00), code_point_t(0xDFFF), code_point_t(0x110000), code_point_t(0xFFFFFFFF) })
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
		for (code_point_t cp : { code_point_t(0x0), code_point_t(0x7F), code_point_t(0x80), code_point_t(0xD7FF), code_point_t(0xE000), code_point_t(0x1F600), code_point_t(0x10FFFF) })
		{
			auto encoded = encode(cp);
			auto begin   = encoded.begin();
			auto decoded = decoder::decode<true>(begin, encoded.end());
			CHECK(!decoded.failed());
			CHECK(decoded.value() == cp);
			CHECK(begin == encoded.end());
		}
	}
}

int main()
{
	test_one_unit();
	test_non_scalar();
	test_round_trip();

	if (failures == 0) { std::printf("pdn_utf32_encoder: ok\n"); }
	return failures == 0 ? 0 : 1;
}