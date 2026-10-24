#ifndef PDN_Header_pdn_from_chars_hex_float
#define PDN_Header_pdn_from_chars_hex_float

#include <type_traits>
#include <bit>
#include <cstdint>
#include <limits>
#include <concepts>
#include <charconv>

namespace pdn::detail
{
	constexpr int digit_char_to_int(char c) noexcept
	{
		if (c >= '0' && c <= '9') return c - '0';
		return -1; // invalid digit character
	}
	constexpr int hex_char_to_int(char c) noexcept
	{
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		return -1; // invalid hex character
	}
}

namespace pdn::detail::charconv
{
	template <typename T>
	concept supported_float = ::std::same_as<T, float> || ::std::same_as<T, double>;

	template <supported_float T>
	using bits_for = ::std::conditional_t<::std::same_as<T, float>, std::uint32_t, std::uint64_t>;

	template <supported_float T>
	constexpr int mantissa_bit_count = ::std::same_as<T, float> ? 23 : 52;

	template <supported_float T>
	constexpr int exponent_bias = ::std::same_as<T, float> ? 127 : 1023;

	template <supported_float T>
	constexpr int max_exponent = ::std::same_as<T, float> ? 127 : 1023;

	template <supported_float T>
	constexpr int min_exponent = ::std::same_as<T, float> ? -126 : -1022;

	template <supported_float T>
	constexpr int sign_bits_bias = ::std::same_as<T, float> ? 31 : 63;

	template <supported_float T>
	constexpr int exponent_bit_bias = ::std::same_as<T, float> ? 23 : 52;

	using mantissa_t = ::std::uint64_t;
	using exponent_t = ::std::int64_t;

	struct [[nodiscard]] parse_mantissa_result
	{
		const char* next;
		// number = mantissa * 2^exponent, mantissa is a 64-bit unsigned integer, exponent is a signed integer
		mantissa_t mantissa;
		exponent_t exponent;
		bool seen_digit;
		bool sticky;
		bool exponent_saturated;
	};

	inline auto parse_mantissa(const char* const first, const char* const last) noexcept -> parse_mantissa_result
	{
		// overflow protection: mantissa_result.exponent + exponent_result.exponent + shift(0~-63) + 63
		constexpr auto exponent_overflow_limit = std::numeric_limits<exponent_t>::max() / 2 - 32 - 4;
		constexpr auto exponent_underflow_limit = std::numeric_limits<exponent_t>::min() / 2 + 32 + 4;
		auto next = first;
		auto mantissa = mantissa_t{};
		auto exponent = exponent_t{};
		auto seen_digit = false;
		auto seen_point = false;
		auto sticky = false;
		auto exponent_saturated = false;

		while (next != last)
		{
			auto c = *next;

			if (c == '.')
			{
				if (seen_point)
				{
					break;
				}
				seen_point = true;
				++next;
				continue;
			}

			auto d = hex_char_to_int(c);
			if (d < 0)
			{
				break;
			}

			seen_digit = true;

			if (mantissa <= (std::numeric_limits<mantissa_t>::max() >> 4))
			{
				mantissa = (mantissa << 4) | static_cast<mantissa_t>(d);
				// adjustment with saturation to avoid underflow
				if (seen_point)
				{
					if (exponent > exponent_underflow_limit)
					{
						exponent -= 4;
					}
					else
					{
						exponent_saturated = true;
					}
				}
			}
			else
			{
				sticky = sticky || d;
				// adjustment with saturation to avoid overflow
				if (!seen_point)
				{
					if (exponent < exponent_overflow_limit)
					{
						exponent += 4;
					}
					else
					{
						exponent_saturated = true;
					}
				}
			}
			++next;
		}

		return { next, mantissa, exponent, seen_digit, sticky, exponent_saturated };
	}

	struct [[nodiscard]] parse_exponent_result
	{
		const char* next;
		exponent_t exponent;
		bool seen_digit;
		bool exponent_saturated;
	};

	inline auto parse_exponent(const char* const first, const char* const last) noexcept -> parse_exponent_result
	{
		// overflow protection: mantissa_result.exponent + exponent_result.exponent + shift(0~-63) + 63
		constexpr auto exponent_overflow_limit = (std::numeric_limits<exponent_t>::max() / 2 - 32 - 9) / 10;
		auto next = first;
		auto exponent = exponent_t{};
		auto seen_digit = false;
		auto seen_p = false;
		auto seen_sign = false;
		auto exponent_saturated = false;

		if (next != last && (*next == 'p' || *next == 'P'))
		{
			++next;
			seen_p = true;
			bool negative = false;
			if (next != last && (*next == '-' || *next == '+'))
			{
				seen_sign = true;
				negative = (*next == '-');
				++next;
			}
			while (next != last)
			{
				auto c = *next;
				auto d = digit_char_to_int(c);
				if (d < 0)
				{
					break;
				}
				seen_digit = true;
				// adjustment with saturation to avoid overflow
				if (exponent < exponent_overflow_limit)
				{
					exponent = exponent * 10 + d;
				}
				else
				{
					exponent_saturated = true;
				}
				++next;
			}
			if (negative)
			{
				exponent = -exponent;
			}
		}
		if (!seen_digit)
		{
			if (seen_p) --next;
			if (seen_sign) --next;
		}
		return { next, exponent, seen_digit, exponent_saturated };
	}

	template <supported_float T>
	auto make_from_chars_hex_float_result(
		T& value,
		const char* const next,
		mantissa_t mantissa, // normal form: 1.n (n = low 63 bits)
		exponent_t exponent,
		bool is_negative,
		bool inherited_sticky) noexcept -> std::from_chars_result
	{
		constexpr auto guard_bit_mask = mantissa_t{ 1 } << (63 - mantissa_bit_count<T>);
		constexpr auto sticky_bit_mask = ~mantissa_t{} >> (mantissa_bit_count<T> +1);

		if (exponent < min_exponent<T>) // subnormal and underflow handling
		{
			++exponent; // 1.n -> 0.m (n = mantissa low 63 bits, m = mantissa all 64 bits)
			auto shift = min_exponent<T> - exponent; // must >= 0
			auto guard = false;
			auto sticky = inherited_sticky;
			if (shift >= mantissa_bit_count<T>)
			{
				// shift >= mantissa_bit_count<T>, it means all the digits at the end are dropped
				if (shift == mantissa_bit_count<T>)
				{
					constexpr auto highest_bit_mask = mantissa_t{ 1 } << 63;
					constexpr auto low63_bit_mask = ~highest_bit_mask;
					guard = mantissa & highest_bit_mask;
					sticky = sticky || (mantissa & low63_bit_mask);
				}
				else
				{
					// shift > mantissa_bit_count<T>, it means all the digits at the end are dropped, and guard is 0
					// mantissa must >= 0, all bits are sticky
					sticky = true;
				}
				mantissa = 0;
				if (!(guard && sticky))
				{
					return { next, ::std::errc::result_out_of_range };
				}
			}
			else
			{
				// highest bit of mantissa is 1, right move(<mantissa_bit_count) not enough to make the high mantissa_bit_count bits zero
				// 0 <= shift < mantissa_bit_count < 64
				const auto lost_bits = shift ? (mantissa & (~mantissa_t{} >> (64 - shift))) : mantissa_t{};
				mantissa >>= shift;
				guard = mantissa & guard_bit_mask;
				sticky = sticky || (mantissa & sticky_bit_mask) || lost_bits;
				// move valid bits to low mantissa_bit_count
				mantissa >>= (64 - mantissa_bit_count<T>);
			}
			exponent = 0; // for subnormal

			const bool round_up = guard && (sticky || (mantissa & 1));
			if (round_up) // round up if needed
			{
				++mantissa;
				if (mantissa == (mantissa_t{ 1 } << mantissa_bit_count<T>))
				{
					// at this point, the carryover in the mantissa exceeds the number of bits the mantissa can represent,
					// so it must be a normal number, and the exponent must be the minimum exponent value.
					exponent = min_exponent<T> + exponent_bias<T>; // apply bias
					mantissa = 0;
				}
			}

			auto sign_bits = is_negative ? (bits_for<T>{ 1 } << sign_bits_bias<T>) : bits_for<T>{ 0 };
			auto exponent_bits = static_cast<bits_for<T>>(exponent) << exponent_bit_bias<T>;
			auto bits = sign_bits | exponent_bits | static_cast<bits_for<T>>(mantissa);
			value = ::std::bit_cast<T>(bits);
			return { next, ::std::errc{} };
		}
		else if (exponent > max_exponent<T>)
		{
			return { next, ::std::errc::result_out_of_range };
		}
		else
		{
			mantissa <<= 1; // remove the leading 1 to align the mantissa to the higher bits
			const bool guard = mantissa & guard_bit_mask;
			const bool sticky = inherited_sticky || (mantissa & sticky_bit_mask);
			mantissa >>= (64 - mantissa_bit_count<T>); // keep only the mantissa bits
			const bool round_up = guard && (sticky || (mantissa & 1));

			if (round_up) // round up if needed
			{
				++mantissa;
				if (mantissa == (mantissa_t{ 1 } << mantissa_bit_count<T>))
				{
					mantissa = 0;
					++exponent;
					if (exponent > max_exponent<T>)
					{
						return { next, ::std::errc::result_out_of_range };
					}
				}
			}
			exponent += exponent_bias<T>; // apply bias
			auto sign_bits = is_negative ? (bits_for<T>{ 1 } << sign_bits_bias<T>) : bits_for<T>{ 0 };
			auto exponent_bits = static_cast<bits_for<T>>(exponent) << exponent_bit_bias<T>;
			auto bits = sign_bits | exponent_bits | static_cast<bits_for<T>>(mantissa);
			value = ::std::bit_cast<T>(bits);
			return { next, ::std::errc{} };
		}
	}
}

namespace pdn::charconv
{
	template <detail::charconv::supported_float T>
	auto from_chars_hex_float(const char* first, const char* last, T& value) noexcept -> ::std::from_chars_result
	{
		using namespace detail::charconv;

		auto next = first;

		auto negative = false;

		if (next != last && *next == '-')
		{
			++next;
			negative = true;
		}

		auto mantissa_result = parse_mantissa(next, last);
		if (!mantissa_result.seen_digit)
		{
			return { first, ::std::errc::invalid_argument };
		}

		next = mantissa_result.next;

		auto exponent_result = parse_exponent(next, last);

		next = exponent_result.next;

		auto mantissa = mantissa_result.mantissa; // value = mantissa * 2^exponent

		if (mantissa == 0)
		{
			value = negative ? static_cast<T>(-0.0) : static_cast<T>(0.0);
			return { next, ::std::errc{} };
		}
		if (mantissa_result.exponent_saturated || exponent_result.exponent_saturated)
		{
			return { next, ::std::errc::result_out_of_range };
		}

		auto shift = int{}; // 0~-63
		while ((mantissa & (mantissa_t{ 1 } << 63)) == 0)
		{
			mantissa <<= 1;
			--shift; // shift cannot overflow
		}
		auto exponent = mantissa_result.exponent + exponent_result.exponent + shift + 63;
		// mantissa is now normalized, with the leading 1 in bit 63
		// value = 1.n * 2^exponent (n is mantissa low 63bit)

		return make_from_chars_hex_float_result(value, next, mantissa, exponent, negative, mantissa_result.sticky);
	}
}

#endif
