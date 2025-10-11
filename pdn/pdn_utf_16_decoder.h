#ifndef PDN_Header_pdn_utf_16_decoder
#define PDN_Header_pdn_utf_16_decoder

#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_utf_16_base.h"

namespace pdn::unicode::utf_16
{
	enum class decode_error_code : ::std::uint16_t
	{
		invalid_code_point = 1, // decode result is not scalar value, but utf-16 will never make this error
		non_code_point,         // decode result is not code point
		incomplete_sequence,    // case: leading surrogate + non trailing surrogate
		trailing_as_start,      // trailing surrogate is first code unit by reading
		unexpected_eof_offset0, // eof when read first code unit
		unexpected_eof_offset1, // case: leading surrogate + EOF
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

			using value_type     = decode_result::value_type;
			using code_unit_type = ::std::remove_reference_t<decltype(*begin)>;
			using ucu_t          = ::std::make_unsigned_t<code_unit_type>; // unsigned code unit type

			decode_result result{};

			if (begin == end) [[unlikely]]
			{
				result.error_code = unexpected_eof_offset0;
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
				if (begin == end) [[unlikely]]
				{
					result.error_code = unexpected_eof_offset1;
					return result;
				}
				auto trailing = ucu_t(*begin);
				if (!is_trailing_surrogate(trailing)) [[unlikely]]
				{
					result.error_code = incomplete_sequence;
					return result;
				}
				result.code_point |= (value_type(trailing) & value_type(0x03FF));
				if (!is_scalar_value(result.value())) [[unlikely]]
				{
					result.error_code = is_code_point(result.value()) ? invalid_code_point : non_code_point;
				}
			}
			else [[unlikely]] // code unit must be trailing surrogate
			{
				result.error_code = trailing_as_start;
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
