#ifndef PDN_Header_pdn_utf_8_decoder
#define PDN_Header_pdn_utf_8_decoder

#include <type_traits>
#include <cstdint>
#include <array>
#include <stdexcept>
#include <utility>

#include "pdn_unicode_base.h"
#include "pdn_utf_8_base.h"

namespace pdn::unicode::utf_8
{
	enum class decode_error_code : ::std::uint16_t
	{
		success,

		not_scalar_value,

		eof_when_read_1st_code_unit,
		eof_when_read_2nd_code_unit,
		eof_when_read_3rd_code_unit,
		eof_when_read_4th_code_unit,
		eof_when_read_5th_code_unit,
		eof_when_read_6th_code_unit,
		requires_utf_8_trailing, // requires trailing and read one which not trailing
		requires_utf_8_leading, // requires leading and read one which not leading
		unsupported_utf_8_leading,

		unknown,
	};

	class decode_result;
	template <bool force_to_next = false>
	inline decode_result decode(auto&& begin, auto end);

	class decode_result final
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
		constexpr auto error() const noexcept
		{
			return error_code;
		}
		constexpr auto distance() const noexcept
		{
			return distance_count;
		}
		constexpr explicit operator bool() const noexcept
		{
			return error() == error_type::success;
		}
		template <bool>
		friend decode_result decode(auto&&, auto);
	private:
		value_type code_point{};
		error_type error_code{ error_type::success };
		count_type distance_count{};
	};
}

namespace pdn::unicode::utf_8::impl_components
{
	// enum class type, show how may code units to a unicode character
	struct code_units_count final
	{
		using enum_type = ::std::uint8_t;
		static constexpr enum_type invalid{ 0 }; // i do not support decode character which need 7 or more code units, and mark it invalid
		static constexpr enum_type unknown{ 0 }; // that because unicode standard 15.0: code point 0 to 10FFFFH, just need 4 code units
		static constexpr enum_type c1{ 1 };
		static constexpr enum_type c2{ 2 };
		static constexpr enum_type c3{ 3 };
		static constexpr enum_type c4{ 4 };
		static constexpr enum_type c5{ 5 }; // ucs4 code point in 0 to 7FFFFFFF, thus it maybe need 5 or 6 code units to encode
		static constexpr enum_type c6{ 6 }; // but ucs4 not specify code point 110000H to 7FFFFFFFH, and unicode do not have these code points
		static constexpr enum_type trail{ 7 }; // mark the trailing units
		static constexpr enum_type unconfirm{ 8 }; // state in implementation(check high 5 bits first)
		enum_type value{ invalid };
		code_units_count() = default;
		code_units_count(const code_units_count&) = default;
		explicit code_units_count(enum_type e) noexcept : value{ e } {}
		explicit constexpr operator enum_type() const noexcept { return value; }
	};

	inline auto first_table() noexcept -> const ::std::array<code_units_count::enum_type, 32>&
	{
		struct first_code_units_count_table final
		{
			::std::array<code_units_count::enum_type, 32> table;
			first_code_units_count_table()
			{
				auto it = table.begin();
				int code = 0;
				for (; code < 0b10'000; ++code, ++it) *it = code_units_count::c1;
				for (; code < 0b11'000; ++code, ++it) *it = code_units_count::trail;
				for (; code < 0b11'100; ++code, ++it) *it = code_units_count::c2;
				for (; code < 0b11'110; ++code, ++it) *it = code_units_count::c3;
				*(it++) = code_units_count::c4;
				*(it++) = code_units_count::unconfirm;
			}
		};
		static const first_code_units_count_table foo;
		return foo.table;
	}

	inline auto second_table() noexcept -> const ::std::array<code_units_count::enum_type, 8>&
	{
		// define F+N = 0b1111'1000 + N
		static const ::std::array<code_units_count::enum_type, 8> table
		{
			code_units_count::c5,      // F+0b000
			code_units_count::c5,      // F+0b001
			code_units_count::c5,      // F+0b010
			code_units_count::c5,      // F+0b011
			code_units_count::c6,      // F+0b100
			code_units_count::c6,      // F+0b101
			code_units_count::unknown, // F+0b110
			code_units_count::unknown, // F+0b111
		};
		return table;
	}

	inline code_unit_t high_5_bits(code_unit_t c) noexcept
	{
		return c >> 3;
	}

	inline code_unit_t low_3_bits(code_unit_t c) noexcept
	{
		return c & code_unit_t(0b0111);
	}

	inline auto map_first_table(code_unit_t c) noexcept -> code_units_count::enum_type
	{
		return first_table()[high_5_bits(c)];
	}

	inline auto map_second_table(code_unit_t c) noexcept -> code_units_count::enum_type
	{
		return second_table()[low_3_bits(c)];
	}
}

namespace pdn::unicode::utf_8
{
	template <bool reach_next_code_point>
	inline decode_result decode(auto&& begin, auto end)
	{
		using enum decode_error_code;
		using impl_components::map_first_table;
		using impl_components::map_second_table;

		using value_type = decode_result::value_type;
		using code_unit_type = ::std::remove_reference_t<decltype(*begin)>;
		using ucu_t = ::std::make_unsigned_t<code_unit_type>; // unsigned code unit type

		// domain: [1, 5] int
		auto process_trailing = [](int trailing_count, auto& begin, auto end) -> decode_result
		{
			decode_result result{};
			for (int i{ 1 }; i <= trailing_count; ++i)
			{
				++begin;
				++result.distance_count;
				if (begin == end)
				{
					switch (i)
					{
					case 1:
						result.error_code = eof_when_read_2nd_code_unit;
						break;
					case 2:
						result.error_code = eof_when_read_3rd_code_unit;
						break;
					case 3:
						result.error_code = eof_when_read_4th_code_unit;
						break;
					case 4:
						result.error_code = eof_when_read_5th_code_unit;
						break;
					case 5:
						result.error_code = eof_when_read_6th_code_unit;
						break;
					default:
						// do not catch this exception. if throwing there, must be some error in caller(utf_8::decode) or be hacked
						throw ::std::domain_error("[pdn] inner utf-8 decoder error: trailing_count not belong to [1, 5] int");
						break;
					}
					return result;
				}
				auto c = ucu_t(*begin);
				if (utf_8::is_trailing(c))
				{
					result.code_point |= (((value_type(c) & 0x3F) << ((trailing_count - i) * 6)));
				}
				else
				{
					result.error_code = requires_utf_8_trailing;
					break;
				}
			}
			return result;
		};

		decode_result result{};

		if (begin == end)
		{
			result.error_code = eof_when_read_1st_code_unit;
			return result;
		}

		using impl_components::code_units_count;
		
		switch (auto c = ucu_t(*begin); map_first_table(c))
		{
		case code_units_count::c1:
			result.code_point = value_type(c);
			if constexpr (reach_next_code_point)
			{
				++begin;
				++result.distance_count;
			}
			return result;
		case code_units_count::c2:
			(result = process_trailing(1, begin, end)).code_point |= ((value_type(c) & 0x1F) << (6 * 1));
			break;
		case code_units_count::c3:
			(result = process_trailing(2, begin, end)).code_point |= ((value_type(c) & 0x0F) << (6 * 2));
			break;
		case code_units_count::c4:
			(result = process_trailing(3, begin, end)).code_point |= ((value_type(c) & 0x07) << (6 * 3));
			break;
		case code_units_count::unconfirm:
			switch (map_second_table(c))
			{
			case code_units_count::c5:
				(result = process_trailing(4, begin, end)).code_point |= ((value_type(c) & 0x03) << (6 * 4));
				break;
			case code_units_count::c6:
				(result = process_trailing(5, begin, end)).code_point |= ((value_type(c) & 0x01) << (6 * 5));
				break;
			case code_units_count::unknown:
				result.error_code = unsupported_utf_8_leading;
				if constexpr (reach_next_code_point)
				{
					++begin;
					++result.distance_count;
				}
				return result;
			default:
				// mark c1, c2, c3, c4, trail and unconfirm not in second table, this branch will never be executed.
				result.error_code = requires_utf_8_leading;
				if constexpr (reach_next_code_point)
				{
					++begin;
					++result.distance_count;
				}
				return result;
			}
			break;
		case code_units_count::trail:
		default:
			// mark c5, c6 and invalid not in first table, this branch only for mark trail.
			result.error_code = requires_utf_8_leading;
			if constexpr (reach_next_code_point)
			{
				++begin;
				++result.distance_count;
			}
			return result;
		}

		if (result && !is_scalar_value(result.value()))
		{
			result.error_code = not_scalar_value;
		}

		if constexpr (reach_next_code_point)
		{
			++begin;
			++result.distance_count;
		}

		return result;
	}
}

#endif
