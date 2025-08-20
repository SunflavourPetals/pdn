#ifndef PDN_Header_pdn_utf_32_decoder
#define PDN_Header_pdn_utf_32_decoder

#include <type_traits>

#include "pdn_unicode_base.h"
#include "pdn_utf_32_base.h"

namespace pdn::unicode::utf_32
{
	enum class decode_error_code : ::std::uint8_t
	{
		not_scalar_value = 1,    // decode result is not Unicode scalar value
		eof_when_read_code_unit, // eof when read first code unit
	};

	class decoder;

	class [[nodiscard("decode_result should be processed")]] decode_result final
	{
	public:
		using value_type = code_point_t;
		using error_type = decode_error_code;
		using count_type = ::std::uint16_t;
		using bool_type  = ::std::uint8_t;
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
			return static_cast<bool>(is_failed);
		}
		constexpr explicit operator bool() const noexcept
		{
			return !failed();
		}
	private:
		value_type code_point{};
		count_type distance_count{};
		bool_type  is_failed{};
		error_type error_code{}; // valid only on failure

		constexpr void set_error(error_type code) noexcept
		{
			is_failed  = true;
			error_code = code;
		}
		friend class decoder;
	};

	class decoder
	{
	public:
		template <bool reach_next_code_point>
		static auto decode(auto&& begin, auto end) -> decode_result
		{
			using enum decode_error_code;

			using code_unit_type = ::std::remove_reference_t<decltype(*begin)>;
			using ucu_t          = ::std::make_unsigned_t<code_unit_type>; // unsigned code unit type
			
			decode_result result{};

			if (begin == end)
			{
				result.set_error(eof_when_read_code_unit);
				return result;
			}

			result.code_point = ucu_t(*begin);
			if (!is_scalar_value(result.value()))
			{
				result.set_error(not_scalar_value);
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
