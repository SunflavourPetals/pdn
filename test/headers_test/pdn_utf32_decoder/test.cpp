#include <array>
#include <cstdio>

#include <pdn/pdn_utf32_decoder.h>

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
	using units = std::array<char32_t, 2>;

	template <bool reach_next_code_point>
	decode_result run(units const& u, std::size_t used)
	{
		auto begin = u.begin();
		return decoder::decode<reach_next_code_point>(begin, u.begin() + used);
	}

	void test_enum()
	{
		static_assert(static_cast<int>(decode_error_code::invalid_code_point) == 1);
		static_assert(static_cast<int>(decode_error_code::unexpected_eof) == 3);
	}

	void test_valid()
	{
		units u{ char32_t(0x41) };
		auto r = run<false>(u, 1);
		CHECK(!r.failed());
		CHECK(r.value() == 0x41u);
		CHECK(r.distance() == 0u);
		CHECK(!decoder::is_reaching_next<false>(r)); // one unit per decode: the caller steps
		CHECK(run<true>(u, 1).distance() == 1u);

		units umax{ char32_t(0x10FFFF) };
		auto rmax = run<false>(umax, 1);
		CHECK(!rmax.failed());
		CHECK(rmax.value() == 0x10FFFFu);

		units ubmp{ char32_t(0xFFFF) };
		CHECK(run<false>(ubmp, 1).value() == 0xFFFFu);
	}

	void test_errors()
	{
		units us{ char32_t(0xD800) }; // surrogate: not a scalar value
		auto rs = run<false>(us, 1);
		CHECK(rs.errc() == decode_error_code::invalid_code_point);
		CHECK(rs.distance() == 0u);
		CHECK(!decoder::is_reaching_next<false>(rs));

		units ut{ char32_t(0xDFFF) };
		CHECK(run<false>(ut, 1).errc() == decode_error_code::invalid_code_point);

		units ub{ char32_t(0x110000) }; // beyond the code point range
		CHECK(run<false>(ub, 1).errc() == decode_error_code::non_code_point);

		units uw{ char32_t(0xFFFFFFFF) };
		CHECK(run<false>(uw, 1).errc() == decode_error_code::non_code_point);

		units uempty{};
		auto re = run<false>(uempty, 0);
		CHECK(re.errc() == decode_error_code::unexpected_eof);
		CHECK(!decoder::is_reaching_next<false>(re));
	}
}

int main()
{
	test_enum();
	test_valid();
	test_errors();

	if (failures == 0) { std::printf("pdn_utf32_decoder: ok\n"); }
	return failures == 0 ? 0 : 1;
}