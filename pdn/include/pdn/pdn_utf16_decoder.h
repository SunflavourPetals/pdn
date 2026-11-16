#ifndef PDN_Header_pdn_utf16_decoder
#define PDN_Header_pdn_utf16_decoder

#include <type_traits>
#include <cstdint>

#include "pdn_unicode_base.h"
#include "pdn_stoppable_concept.h"

namespace pdn::unicode::utf16
{
	// Decodes one code point from a range of code units.
	// reach_next_code_point == false stops on the last code unit of the decoded sequence, so the
	// caller advances once to leave it; that is what the code point iterator relies on.
	//
	// invalid_code_point and non_code_point exist for parity with the utf-8 and utf-32 decoders,
	// but this decoder never produces them: a well formed surrogate pair always folds into
	// U+10000..U+10FFFF, which is a scalar value by construction.
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
				// The first two exclusions cannot trigger here (see the enum above); they only keep
				// this predicate shaped like the utf-8 and utf-32 ones.
				using enum decode_error_code;
				return r.distance() > 0 && r.errc() != invalid_code_point && r.errc() != non_code_point;
			}
		}
		template <bool reach_next_code_point>
		static auto decode(auto&& begin, auto end) -> decode_result
		{
			return decode_impl<reach_next_code_point>(begin, end);
		}
	private:
		template <bool reach_next_code_point>
		static auto decode_impl(auto& begin, auto end) -> decode_result
		{
			using enum decode_error_code;

			using value_type     = decode_result::value_type;
			using code_unit_type = ::std::remove_reference_t<decltype(*begin)>;
			using ucu_t          = ::std::make_unsigned_t<code_unit_type>; // unsigned code unit type

			auto result = decode_result{};

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
				if constexpr (pdn::concepts::stoppable<decltype(begin)>)
				{
					if (begin.stop_requested()) [[unlikely]]
					{
						result.error_code = incomplete_sequence;
						return result;
					}
				}
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
		static void to_next(auto& begin, decode_result& result)
		{
			++begin;
			++result.distance_count;
		}
	};

	template <bool reach_next_code_point>
	auto decode(auto&& begin, auto end) -> decode_result
	{
		return decoder::decode<reach_next_code_point>(begin, ::std::move(end));
	}
}

#endif
