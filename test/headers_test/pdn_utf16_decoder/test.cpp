#include <pdn/pdn_utf16_decoder.h>

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
using namespace pdn::unicode::utf16;

namespace
{
	using units = std::array<char16_t, 4>;

	template <bool reach_next_code_point>
	decode_result run(units const& u, std::size_t used)
	{
		auto begin = u.begin();
		return decoder::decode<reach_next_code_point>(begin, u.begin() + used);
	}

	void test_enum()
	{
		static_assert(static_cast<int>(decode_error_code::invalid_code_point) == 1);
		static_assert(static_cast<int>(decode_error_code::unexpected_eof_offset1) == 6);
	}

	void test_bmp()
	{
		units u{ char16_t(0x41) };
		auto r = run<false>(u, 1);
		CHECK(!r.failed());
		CHECK(r.value() == 0x41u);
		CHECK(r.distance() == 0u);
		CHECK(!decoder::is_reaching_next<false>(r));

		CHECK(run<true>(u, 1).distance() == 1u);

		units uf{ char16_t(0xFFFF) };
		CHECK(run<false>(uf, 1).value() == 0xFFFFu);
	}

	void test_pair()
	{
		units u{ char16_t(0xD83D), char16_t(0xDE00) }; // U+1F600
		auto r = run<false>(u, 2);
		CHECK(!r.failed());
		CHECK(r.value() == 0x1F600u);
		CHECK(r.distance() == 1u);
		CHECK(decoder::is_reaching_next<false>(r));
		CHECK(run<true>(u, 2).distance() == 2u);

		units umax{ char16_t(0xDBFF), char16_t(0xDFFF) }; // U+10FFFF
		auto rmax = run<false>(umax, 2);
		CHECK(!rmax.failed());
		CHECK(rmax.value() == 0x10FFFFu);

		units umin{ char16_t(0xD800), char16_t(0xDC00) }; // U+10000
		CHECK(run<false>(umin, 2).value() == 0x10000u);
	}

	void test_errors()
	{
		units ueof{ char16_t(0xD83D) }; // leading surrogate then end of input
		auto re = run<false>(ueof, 1);
		CHECK(re.errc() == decode_error_code::unexpected_eof_offset1);
		CHECK(re.distance() == 1u);
		CHECK(decoder::is_reaching_next<false>(re)); // nothing left to skip

		units ut{ char16_t(0xDC00) }; // trailing surrogate as first unit
		auto rt = run<false>(ut, 1);
		CHECK(rt.errc() == decode_error_code::trailing_as_start);
		CHECK(rt.distance() == 0u);
		CHECK(!decoder::is_reaching_next<false>(rt));

		units ui{ char16_t(0xD83D), char16_t(0x41) }; // lead followed by a non trailing unit
		auto ri = run<false>(ui, 2);
		CHECK(ri.errc() == decode_error_code::incomplete_sequence);
		CHECK(ri.distance() == 1u);
		CHECK(decoder::is_reaching_next<false>(ri)); // the second unit was not consumed

		units uempty{};
		CHECK(run<false>(uempty, 0).errc() == decode_error_code::unexpected_eof_offset0);
	}

	// The two surrogate errors are unreachable for a well formed pair, so a valid pair must
	// report success and is_reaching_next must stay true even at the end of the buffer.
	void test_no_surrogate_error_for_valid_pairs()
	{
		units u{ char16_t(0xD800), char16_t(0xDC00) };
		auto r = run<false>(u, 2);
		CHECK(!r.failed());
		CHECK(r.errc() != decode_error_code::invalid_code_point);
		CHECK(r.errc() != decode_error_code::non_code_point);
	}
}

int main()
{
	test_enum();
	test_bmp();
	test_pair();
	test_errors();
	test_no_surrogate_error_for_valid_pairs();

	if (failures == 0) { std::printf("pdn_utf16_decoder: ok\n"); }
	return failures == 0 ? 0 : 1;
}