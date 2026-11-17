// Review target: pdn/include/pdn/pdn_from_chars_hex_float.h
//
// Coverage:
//   1. basic hexadecimal values for double: fractions, trailing/leading point, exponent signs,
//      upper case 'P';
//   2. double boundaries: smallest subnormal, largest finite value, and the values that must
//      report result_out_of_range (overflow and underflow to zero);
//   3. the same boundaries for float (the template has per-type constants);
//   4. error reporting: invalid_argument for a leading '+', an empty string and a non-digit,
//      success with a pointer stopped before a dangling 'p', value untouched on any error;
//   5. differential check against strtod (an independent implementation) over a table of tricky
//      inputs, including sticky digits beyond 64 bits and rounding ties;
//   6. round trip: printf("%a") output parsed back to the same bits.
//
// The library never calls this header itself (it is opt-in through PDN_USER_FROM_CHARS_FLOAT),
// so these tests are the only thing keeping it from rotting.

#include <pdn/pdn_from_chars_hex_float.h>

#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <bit>
#include <limits>
#include <string>
#include <type_traits>

namespace
{
	template <typename T>
	std::uint64_t bits_of(T v)
	{
		if constexpr (std::is_same_v<T, float>)
		{
			return std::bit_cast<std::uint32_t>(v);
		}
		else
		{
			return std::bit_cast<std::uint64_t>(v);
		}
	}

	template <typename T>
	T parse(const std::string& s, std::errc& ec, std::ptrdiff_t& consumed)
	{
		T v{};
		const auto r = pdn::charconv::from_chars_hex_float(s.data(), s.data() + s.size(), v);
		ec = r.ec;
		consumed = r.ptr - s.data();
		return v;
	}

	template <typename T>
	void expect_value(const std::string& s, double expected)
	{
		std::errc ec{};
		std::ptrdiff_t consumed = 0;
		const auto v = parse<T>(s, ec, consumed);
		assert(ec == std::errc{});
		assert(consumed == static_cast<std::ptrdiff_t>(s.size()));
		const auto diff = std::fabs(static_cast<double>(v) - expected);
		assert(diff <= std::fabs(expected) * 1e-15);
	}

	template <typename T>
	void expect_bits(const std::string& s, std::uint64_t expected)
	{
		std::errc ec{};
		std::ptrdiff_t consumed = 0;
		const auto v = parse<T>(s, ec, consumed);
		assert(ec == std::errc{});
		assert(consumed == static_cast<std::ptrdiff_t>(s.size()));
		assert(bits_of(v) == expected);
	}

	template <typename T>
	void expect_out_of_range(const std::string& s)
	{
		std::errc ec{};
		std::ptrdiff_t consumed = 0;
		const T sentinel = static_cast<T>(42.5);
		T v = sentinel;
		const auto r = pdn::charconv::from_chars_hex_float(s.data(), s.data() + s.size(), v);
		ec = r.ec;
		consumed = r.ptr - s.data();
		assert(ec == std::errc::result_out_of_range);
		assert(consumed == static_cast<std::ptrdiff_t>(s.size()));
		assert(bits_of(v) == bits_of(sentinel)); // untouched
	}

	void test_basic_f64()
	{
		expect_value<double>("1.8p3", 12.0);
		expect_value<double>("1p0", 1.0);
		expect_value<double>(".8p0", 0.5);
		expect_value<double>("8.p0", 8.0);
		expect_value<double>("1.8p-3", 0.1875);
		expect_value<double>("1P+3", 8.0);
		expect_value<double>("0p0", 0.0);
		expect_bits<double>("-0p0", 0x8000000000000000ull);
		expect_value<double>("ffffffffffffffffp0", 18446744073709551616.0); // 2^64, 64 significant bits
	}

	void test_boundaries_f64()
	{
		expect_bits<double>("1p-1074", 0x0000000000000001ull); // smallest subnormal
		expect_bits<double>("1.8p-1074", 0x0000000000000002ull); // 1.5 ulp, ties to even
		expect_bits<double>("1.fffffffffffffp1023", 0x7FEFFFFFFFFFFFFFull); // largest finite
		expect_bits<double>("1p-1022", 0x0010000000000000ull); // smallest normal
		expect_out_of_range<double>("1p1024");
		expect_out_of_range<double>("1p-1075"); // half of the smallest subnormal, ties to zero
		expect_out_of_range<double>("1.fffffffffffff8p1023"); // rounds past the largest finite value
	}

	void test_boundaries_f32()
	{
		expect_bits<float>("1p-149", 0x00000001ull); // smallest subnormal
		expect_bits<float>("1.8p-149", 0x00000002ull);
		expect_bits<float>("1p-126", 0x00800000ull); // smallest normal
		expect_bits<float>("1.fffffep127", 0x7F7FFFFFull); // largest finite
		expect_out_of_range<float>("1p128");
		expect_out_of_range<float>("1p-150");
	}

	void test_errors_and_pointer()
	{
		{
			std::errc ec{};
			std::ptrdiff_t consumed = -1;
			const auto v = parse<double>("+1.8p3", ec, consumed);
			assert(ec == std::errc::invalid_argument);
			assert(consumed == 0);
			assert(v == 0.0);
		}
		{
			std::errc ec{};
			std::ptrdiff_t consumed = -1;
			parse<double>("", ec, consumed);
			assert(ec == std::errc::invalid_argument);
			assert(consumed == 0);
		}
		{
			std::errc ec{};
			std::ptrdiff_t consumed = -1;
			parse<double>("zzz", ec, consumed);
			assert(ec == std::errc::invalid_argument);
			assert(consumed == 0);
		}
		{
			// "1p" is a valid number followed by a dangling exponent marker: parsing succeeds and
			// stops before 'p'. The lexer turns that into its own "incomplete number" error.
			std::errc ec{};
			std::ptrdiff_t consumed = 0;
			const auto v = parse<double>("1p", ec, consumed);
			assert(ec == std::errc{});
			assert(consumed == 1);
			assert(v == 1.0);
		}
		{
			// value untouched on invalid_argument as well
			const double sentinel = 42.5;
			double v = sentinel;
			const char* s = "nope";
			const auto r = pdn::charconv::from_chars_hex_float(s, s + 4, v);
			assert(r.ec == std::errc::invalid_argument);
			assert(bits_of(v) == bits_of(sentinel));
		}
	}

	void test_against_strtod()
	{
		// strtod parses the same hexadecimal syntax (0x prefix added here) and is an independent
		// implementation to compare against. Only inputs that are in range are compared bitwise.
		const char* table[] = {
			"1.8p3",
			"1.00000000000008p0",  // exactly half an ulp: ties to even
			"1.00000000000018p0",  // one and a half ulp: rounds up
			"1.000000000000081p0", // sticky bit decides
			"1.ffffffffffffffffp0",
			"1.fffffffffffffffp-1022",
			"0.00000000000000000000000000001p0",
			"123456789abcdef0123456789abcdefp-120",
			"ffffffffffffffffffffffffffffffffp-64",
			"1p-1060",
			"1.0000000000001p-1060",
			"1p-1073",
			".00000000000000000001p1023",
		};
		for (const char* s : table)
		{
			std::errc ec{};
			std::ptrdiff_t consumed = 0;
			const auto mine = parse<double>(s, ec, consumed);
			assert(ec == std::errc{});

			const std::string prefixed = std::string("0x") + s;
			char* end = nullptr;
			errno = 0;
			const double ref = std::strtod(prefixed.c_str(), &end);
			assert(end == prefixed.c_str() + prefixed.size());
			assert(errno != ERANGE);
			assert(bits_of(mine) == bits_of(ref));
		}
	}

	void test_printf_a_round_trip()
	{
		const double values[] = {
			0.1, 1.0 / 3.0, 1.0, -2.5, 1e-300, 1e300, std::numeric_limits<double>::max(),
			std::numeric_limits<double>::denorm_min(), std::numeric_limits<double>::min(),
			std::numeric_limits<double>::epsilon(),
		};
		for (const double v : values)
		{
			char text[64]{};
			const int len = std::snprintf(text, sizeof(text), "%a", v);
			// The sign is printed in front of the prefix: "-0x1.4p+1".
			const bool negative = len > 0 && text[0] == '-';
			const char* prefix = text + (negative ? 1 : 0);
			assert(text + len - prefix > 2 && prefix[0] == '0' && (prefix[1] == 'x' || prefix[1] == 'X'));
			const char* const end = text + len;
			const std::string body = std::string(negative ? "-" : "") + std::string(prefix + 2, end);
			std::errc ec{};
			std::ptrdiff_t consumed = 0;
			const auto back = parse<double>(body, ec, consumed);
			assert(ec == std::errc{});
			assert(consumed == static_cast<std::ptrdiff_t>(body.size()));
			assert(bits_of(back) == bits_of(v));
		}
	}
}

int main()
{
	test_basic_f64();
	test_boundaries_f64();
	test_boundaries_f32();
	test_errors_and_pointer();
	test_against_strtod();
	test_printf_a_round_trip();
	return 0;
}
