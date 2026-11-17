#include <pdn/pdn_utf8_decoder.h>

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
	using units = std::array<char8_t, 6>;

	template <bool reach_next_code_point>
	decode_result run(units const& u, std::size_t used)
	{
		auto begin = u.begin();
		return decoder::decode<reach_next_code_point>(begin, u.begin() + used);
	}

	void test_enum()
	{
		static_assert(static_cast<int>(decode_error_code::invalid_code_point) == 1);
		static_assert(static_cast<int>(decode_error_code::unexpected_eof_offset5) == 12);
	}

	// One unit: value is the unit, nothing was consumed, so the caller has to step.
	void test_ascii()
	{
		units u{ char8_t(0x41) };
		auto r = run<false>(u, 1);
		CHECK(!r.failed());
		CHECK(r.value() == 0x41u);
		CHECK(r.distance() == 0u);
		CHECK(!decoder::is_reaching_next<false>(r));

		auto r2 = run<true>(u, 1);
		CHECK(!r2.failed());
		CHECK(r2.distance() == 1u);
	}

	// Two to four units: the iterator stops on the last unit, distance counts the advances.
	void test_multibyte()
	{
		units u2{ char8_t(0xC2), char8_t(0x80) }; // U+0080, smallest two unit sequence
		auto r2 = run<false>(u2, 2);
		CHECK(!r2.failed());
		CHECK(r2.value() == 0x80u);
		CHECK(r2.distance() == 1u);
		CHECK(decoder::is_reaching_next<false>(r2));

		units u3{ char8_t(0xE4), char8_t(0xB8), char8_t(0xAD) }; // U+4E2D
		auto r3 = run<false>(u3, 3);
		CHECK(!r3.failed());
		CHECK(r3.value() == 0x4E2Du);
		CHECK(r3.distance() == 2u);
		CHECK(decoder::is_reaching_next<false>(r3));

		auto r3b = run<true>(u3, 3);
		CHECK(!r3b.failed());
		CHECK(r3b.distance() == 3u);

		units u4{ char8_t(0xF0), char8_t(0x9F), char8_t(0x98), char8_t(0x80) }; // U+1F600
		auto r4 = run<false>(u4, 4);
		CHECK(!r4.failed());
		CHECK(r4.value() == 0x1F600u);
		CHECK(r4.distance() == 3u);

		units umax{ char8_t(0xF4), char8_t(0x8F), char8_t(0xBF), char8_t(0xBF) }; // U+10FFFF
		auto rmax = run<false>(umax, 4);
		CHECK(!rmax.failed());
		CHECK(rmax.value() == 0x10FFFFu);
	}

	// Regression: an overlong sequence is reported once and the caller must still step over it,
	// otherwise the last byte is decoded again as a stray trailing unit.
	void test_overlong()
	{
		units u3{ char8_t(0xE0), char8_t(0x80), char8_t(0x80) };
		auto r3 = run<false>(u3, 3);
		CHECK(r3.errc() == decode_error_code::non_shortest_sequence);
		CHECK(r3.distance() == 2u);
		CHECK(!decoder::is_reaching_next<false>(r3));

		units u2{ char8_t(0xC0), char8_t(0x80) };
		auto r2 = run<false>(u2, 2);
		CHECK(r2.errc() == decode_error_code::non_shortest_sequence);
		CHECK(r2.distance() == 1u);
		CHECK(!decoder::is_reaching_next<false>(r2));
	}

	void test_errors()
	{
		units us{ char8_t(0xED), char8_t(0xA0), char8_t(0x80) }; // surrogate U+D800
		CHECK(run<false>(us, 3).errc() == decode_error_code::invalid_code_point);

		units ub{ char8_t(0xF4), char8_t(0x90), char8_t(0x80), char8_t(0x80) }; // U+110000
		CHECK(run<false>(ub, 4).errc() == decode_error_code::non_code_point);

		units ut{ char8_t(0x80) };
		auto rt = run<false>(ut, 1);
		CHECK(rt.errc() == decode_error_code::trailing_as_start);
		CHECK(!decoder::is_reaching_next<false>(rt));

		units uf{ char8_t(0xFE) };
		CHECK(run<false>(uf, 1).errc() == decode_error_code::unsupported_leading);

		units ue{ char8_t(0xE4), char8_t(0xB8) }; // truncated three unit sequence
		auto re = run<false>(ue, 2);
		CHECK(re.errc() == decode_error_code::unexpected_eof_offset2);
		CHECK(decoder::is_reaching_next<false>(re)); // at end: nothing left to skip

		units ui{ char8_t(0xE4), char8_t(0x41) }; // lead followed by a non trailing unit
		auto ri = run<false>(ui, 2);
		CHECK(ri.errc() == decode_error_code::incomplete_sequence);
		CHECK(ri.distance() == 1u);
		CHECK(decoder::is_reaching_next<false>(ri)); // 'A' was not consumed

		units uempty{};
		CHECK(run<false>(uempty, 0).errc() == decode_error_code::unexpected_eof_offset0);
	}

	// The code point iterator shape: decode, then step only when is_reaching_next is false.
	void test_iterator_loop()
	{
		units u{ char8_t(0x41), char8_t(0xE4), char8_t(0xB8), char8_t(0xAD), char8_t(0x80) };
		auto begin = u.begin();
		auto end   = u.begin() + 5;

		auto first = decoder::decode<false>(begin, end);
		CHECK(first.value() == 0x41u);
		if (!decoder::is_reaching_next<false>(first)) { ++begin; }

		auto second = decoder::decode<false>(begin, end);
		CHECK(second.value() == 0x4E2Du);
		if (!decoder::is_reaching_next<false>(second)) { ++begin; }

		// A stray trailing unit is reported one byte at a time; each report leaves the iterator on
		// that byte, so the caller has to step over it before the next decode.
		auto third = decoder::decode<false>(begin, end);
		CHECK(third.errc() == decode_error_code::trailing_as_start);
		CHECK(begin == end - 2);
		if (!decoder::is_reaching_next<false>(third)) { ++begin; }
		CHECK(begin == end - 1);

		auto fourth = decoder::decode<false>(begin, end);
		CHECK(fourth.errc() == decode_error_code::trailing_as_start);
		if (!decoder::is_reaching_next<false>(fourth)) { ++begin; }

		CHECK(begin == end);
	}
}

int main()
{
	test_enum();
	test_ascii();
	test_multibyte();
	test_overlong();
	test_errors();
	test_iterator_loop();

	if (failures == 0) { std::printf("pdn_utf8_decoder: ok\n"); }
	return failures == 0 ? 0 : 1;
}