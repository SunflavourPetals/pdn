#ifndef PDN_Header_pdn_utf_16_decoder
#define PDN_Header_pdn_utf_16_decoder

#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_utf_16_base.h"

namespace pdn::unicode::utf_16
{
	enum class decode_error_code : ::std::uint16_t
	{
		not_scalar_value = 1,             // decode result is not Unicode scalar value, But utf-16 will never make this error
		eof_when_read_code_unit,          // eof when read first code unit
		alone_trailing_surrogate,         // trailing surrogate is first code unit by reading
		eof_when_read_trailing_surrogate, // case: leading surrogate + EOF
		requires_trailing_surrogate,      // case: leading surrogate + non trailing surrogate
	};

	class decoder;

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

	class decoder
	{
	public:
		template <bool reach_next_code_point>
		static auto decode(auto&& begin, auto end) -> decode_result
		{
			using enum decode_error_code;

			using value_type     = decode_result::value_type;
			using code_unit_type = ::std::remove_reference_t<decltype(*begin)>;
			using ucu_t          = ::std::make_unsigned_t<code_unit_type>; // unsigned code unit type

			decode_result result{};

			if (begin == end)
			{
				result.error_code = eof_when_read_code_unit;
				return result;
			}

			result.code_point = ucu_t(*begin);

			if (is_non_surrogate(result.code_point))
			{
				// result.code_point must be unicode scalar value
				if constexpr (reach_next_code_point) { to_next(begin, result); }
				return result;
			}
			else if (is_leading_surrogate(result.code_point))
			{
				result.code_point = ((result.code_point & value_type(0x03FF)) << 10) + value_type(0x10000U);
				to_next(begin, result);
				if (begin == end)
				{
					result.error_code = eof_when_read_trailing_surrogate;
					return result;
				}
				auto trailing = ucu_t(*begin);
				if (!is_trailing_surrogate(trailing))
				{
					result.error_code = requires_trailing_surrogate;
					return result;
				}
				result.code_point |= (value_type(trailing) & value_type(0x03FF));
				if (!is_scalar_value(result.value()))
				{
					result.error_code = not_scalar_value;
				}
			}
			else // code unit must be trailing surrogate
			{
				result.error_code = alone_trailing_surrogate;
			}

			if constexpr (reach_next_code_point) { to_next(begin, result); }

			return result;
		}
	private:
		static void to_next(auto& begin, decode_result& result)
		{
			++begin;
			++result.distance_count;
		}
	};

	template <bool reach_next_code_point>
	inline auto decode(auto&& begin, auto end) -> decode_result
	{
		return decoder::decode<reach_next_code_point>(begin, ::std::move(end));
	}
}

#endif
