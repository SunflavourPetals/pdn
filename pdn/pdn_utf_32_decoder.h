#ifndef PDN_Header_pdn_utf_32_decoder
#define PDN_Header_pdn_utf_32_decoder

#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_utf_32_base.h"

namespace pdn::unicode::utf_32
{
	enum class decode_error_code : ::std::uint16_t
	{
		success,

		not_scalar_value, // decode result is not Unicode scalar value

		eof_when_read_code_unit, // eof when read first code unit

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

	template <bool reach_next_code_point>
	inline decode_result decode(auto&& begin, auto end)
	{
		using code_unit_type = ::std::remove_reference_t<decltype(*begin)>;
		using ucu_t = ::std::make_unsigned_t<code_unit_type>; // unsigned code unit type
		using enum decode_error_code;

		decode_result result{};

		if (begin == end)
		{
			result.error_code = eof_when_read_code_unit;
			return result;
		}

		result.code_point = ucu_t(*begin);
		if (!is_scalar_value(result.value()))
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
