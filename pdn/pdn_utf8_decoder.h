#ifndef PDN_Header_pdn_utf8_decoder
#define PDN_Header_pdn_utf8_decoder

#include <type_traits>
#include <cstdint>
#include <array>
#include <utility>
#ifndef __cpp_lib_unreachable
#include <cassert>
#endif

#include "pdn_unicode_base.h"
#include "pdn_utf8_base.h"

namespace pdn::unicode::utf8
{
	enum class decode_error_code : ::std::uint16_t
	{
		invalid_code_point = 1, // decode result is not scalar value
		non_code_point,         // decode result is not code point
		incomplete_sequence,    // requires trailing and read one which not trailing
		trailing_as_start,      // for 0b10nn'nnnn
		unsupported_leading,    // for 0b1111'1110 and 0b1111'1111
		non_shortest_sequence,   // invalid utf-8 sequence, such as using two bytes to represent values in the range U+0000 to U+007F
		unexpected_eof_offset0,
		unexpected_eof_offset1,
		unexpected_eof_offset2,
		unexpected_eof_offset3,
		unexpected_eof_offset4,
		unexpected_eof_offset5,
	};

	class decoder;

	class [[nodiscard("decode_result should be processed")]] decode_result final
	{
	public:
		using value_type = code_point_t;
		using error_type = decode_error_code;
		using count_type = ::std::uint16_t;
	public:
		constexpr auto value() const noexcept
		{
			return code_point;
		}
		constexpr auto errc() const noexcept
		{
			return error_code;
		}
		constexpr auto distance() const noexcept
		{
			return distance_count;
		}
		constexpr auto failed() const noexcept
		{
			return error_code != error_type{};
		}
		constexpr explicit operator bool() const noexcept
		{
			return !failed();
		}
		friend class decoder;
	private:
		value_type code_point{};
		count_type distance_count{};
		error_type error_code{};
	};
}

namespace pdn::unicode::utf8::impl_components
{
	// decoder::decode dependent code_unit_count.
	// modifications require corresponding updates to decoder::decode.
	enum class code_unit_count : ::std::uint8_t
	{
		// encoding a code point requires up to 4 code units,
		// with the code_unit_count supporting a maximum of 4 code units.
		invalid,
		c1,
		c2,
		c3,
		c4,
		trail, // mark the trailing units
	};

	// decoder::decode dependent code_unit_count_ex.
	// modifications require corresponding updates to decoder::decode.
	enum class code_unit_count_ex : ::std::uint8_t
	{
		// i do not support decode character which need 7 or more code units, and mark it invalid
		// that because unicode standard 15.0 code point range from 0 to 10FFFFH, just need 4 code units.
		invalid,
		c5,
		c6
	};

	constexpr auto high_5_bits(code_unit_t c) noexcept -> code_unit_t
	{
		return c >> 3;
	}

	constexpr auto low_3_bits(code_unit_t c) noexcept -> code_unit_t
	{
		return c & code_unit_t(0b0111);
	}

	struct first_code_units_count_table final
	{
		using value_type = code_unit_count;
		using table_type = ::std::array<value_type, 32>;
		using enum code_unit_count;
		table_type table
		{
			c1, c1, c1, c1, c1, c1, c1, c1, c1, c1, c1, c1, c1, c1, c1, c1, // 16*c1
			trail, trail, trail, trail, trail, trail, trail, trail,         // 8*trail
			c2, c2, c2, c2, // 4*c2
			c3, c3,         // 2*c3
			c4,             // 1*c4
			invalid,        // 1*invalid
		};
		constexpr value_type operator[](code_unit_t c) const noexcept
		{
			return table[high_5_bits(c)];
		}
	};

	struct second_code_units_count_table final
	{
		using value_type = code_unit_count_ex;
		using table_type = ::std::array<value_type, 8>;
		using enum code_unit_count_ex;
		table_type table
		{
			c5, c5, c5, c5,   // 0b1111'1000 ~ 0b1111'1011
			c6, c6,           // 0b1111'1100 ~ 0b1111'1101
			invalid, invalid, // 0b1111'1110 ~ 0b1111'1111
		};
		constexpr value_type operator[](code_unit_t c) const noexcept
		{
			return table[low_3_bits(c)];
		}
	};
}

namespace pdn::unicode::utf8
{
	class decoder
	{
	public:
		using result_type = decode_result;

		template <bool reach_next_code_point>
		static constexpr bool is_reaching_next(decode_result r) noexcept
		{
			if constexpr (reach_next_code_point)
			{
				return true;
			}
			else
			{
				using enum decode_error_code;
				return r.distance() > 0 && r.errc() != invalid_code_point && r.errc() != non_code_point;
			}
		}
		template <bool reach_next_code_point>
		static auto decode(auto&& begin, auto end) -> decode_result
		{
			using enum decode_error_code;

			using value_type = decode_result::value_type;
			using code_unit_type = ::std::remove_reference_t<decltype(*begin)>;
			using ucu_t = ::std::make_unsigned_t<code_unit_type>; // unsigned code unit type

			decode_result result{};

			if (begin == end) [[unlikely]]
			{
				result.error_code = unexpected_eof_offset0;
				return result;
			}

			static constexpr impl_components::first_code_units_count_table  cuc1st_tab{};
			static constexpr impl_components::second_code_units_count_table cuc2nd_tab{};

			switch (auto c = ucu_t(*begin); cuc1st_tab[c]) // switch covers all possible enum values.
			{
			using enum impl_components::code_unit_count;
			case c1:
				result.code_point = value_type(c);
				if constexpr (reach_next_code_point) { to_next(begin, result); }
				// no need to check is it a Unicode scalar value, because it must be
				return result;
			case c2: process_remaining<1>(result, begin, end, c); break;
			case c3: process_remaining<2>(result, begin, end, c); break;
			case c4: process_remaining<3>(result, begin, end, c); break;
			[[unlikely]] case invalid:
				switch (cuc2nd_tab[c]) // switch covers all possible enum values.
				{
				using enum impl_components::code_unit_count_ex;
				case c5: process_remaining<4>(result, begin, end, c); break;
				case c6: process_remaining<5>(result, begin, end, c); break;
				case invalid:
					result.error_code = unsupported_leading;
					if constexpr (reach_next_code_point) { to_next(begin, result); }
					return result;
				}
				break;
			[[unlikely]] case trail:
				result.error_code = trailing_as_start;
				if constexpr (reach_next_code_point) { to_next(begin, result); }
				return result;
			}

			if (result)
			{
				// 1 code unit : 0x0000 ~ 0x007F;
				// 2 code units: 0x0080 ~ 0x07FF;
				// 3 code units: 0x0800 ~ 0xFFFF;
				// 4 code units: 0x1'0000 ~ 0x10'FFFF
				static constexpr ::std::array<::std::uint_least32_t, 4> min_valid{ 0, 0x0080, 0x0800, 0x1'0000 };
				if (!is_scalar_value(result.value())) [[unlikely]]
				{
					result.error_code = is_code_point(result.value()) ? invalid_code_point : non_code_point;
				}
				else if (result.value() < min_valid[result.distance()]) [[unlikely]] // distance eq count(code unit) - 1 now
				{
					result.error_code = non_shortest_sequence;
				}
			}

			if constexpr (reach_next_code_point) { to_next(begin, result); }

			return result;
		}
	private:
		class helper final
		{
		private:
			// trailing_count [1, 5]
			static auto process_trailing(int trailing_count, auto& begin, auto end) -> decode_result
			{
				using enum decode_error_code;

				using value_type = decode_result::value_type;
				using code_unit_type = ::std::remove_reference_t<decltype(*begin)>;
				using ucu_t = ::std::make_unsigned_t<code_unit_type>; // unsigned code unit type

				decode_result result{};
				for (int i = 1; i <= trailing_count; ++i)
				{
					to_next(begin, result);
					if (begin == end) [[unlikely]]
					{
						result.error_code = to_unexpected_eof_errc(i);
						return result;
					}
					auto c = ucu_t(*begin);
					if (utf8::is_trailing(c)) [[likely]]
					{
						result.code_point |= ((value_type(c) & 0x3F) << ((trailing_count - i) * 6));
					}
					else
					{
						result.error_code = incomplete_sequence;
						return result;
					}
				}
				return result;
			}
			static constexpr decode_error_code to_unexpected_eof_errc(int i)
			{
				switch (i)
				{
				using enum decode_error_code;
				case 1: return unexpected_eof_offset1;
				case 2: return unexpected_eof_offset2;
				case 3: return unexpected_eof_offset3;
				case 4: return unexpected_eof_offset4;
				case 5: return unexpected_eof_offset5;
				default:
#ifdef __cpp_lib_unreachable
					std::unreachable();
#else
					assert(false);
#endif
				}
				return decode_error_code{};
			}
		public:
			template <int trailing_count> // trailing_count [1, 5]
			static auto process_trailing(auto& begin, auto end) -> decode_result
			{
				static_assert(trailing_count >= 1 && trailing_count <= 5, "[pdn] trailing_count not belong to [1, 5] int");
				return process_trailing(trailing_count, begin, end);
			}
			static void to_next(auto& begin, decode_result& result)
			{
				++begin;
				++result.distance_count;
			}
		};
		template <int trailing_count>
		static void process_remaining(decode_result& result, auto& begin, auto end, decode_result::value_type c)
		{
			static_assert(trailing_count >= 1 && trailing_count <= 5, "[pdn] trailing_count not belong to [1, 5] int");
			constexpr auto mask     = std::array{ 0x1F, 0x0F, 0x07, 0x03, 0x01 }[trailing_count - 1];
			constexpr auto distance = 6 * trailing_count;
			result = helper::process_trailing<trailing_count>(begin, end);
			result.code_point |= (c & mask) << distance;
		};
		static void to_next(auto& begin, decode_result& result)
		{
			helper::to_next(begin, result);
		}
	};

	template <bool reach_next_code_point>
	inline auto decode(auto&& begin, auto end) -> decode_result
	{
		return decoder::decode<reach_next_code_point>(begin, ::std::move(end));
	}
}

#endif
