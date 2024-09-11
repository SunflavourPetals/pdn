#ifndef PDN_Header_pdn_utf_16_encoder
#define PDN_Header_pdn_utf_16_encoder

#include <array>
#include <utility>

#include "pdn_unicode_base.h"
#include "pdn_utf_16_base.h"

namespace pdn::unicode::utf_16
{
	enum class encode_error_code
	{
		success,

		not_scalar_value,

		unknown,
	};

	class encode_result final
	{
	public:
		using code_unit_sequence_type = ::std::array<code_unit_t, 2>;
		using size_type = unsigned;
		using error_type = encode_error_code;
	public:
		constexpr auto size() const noexcept
		{
			return sequence_size;
		}
		constexpr auto error() const noexcept
		{
			return error_code;
		}
		constexpr auto begin() const noexcept
		{
			return sequence.begin();
		}
		constexpr auto begin() noexcept
		{
			return sequence.begin();
		}
		constexpr auto end() const noexcept
		{
			return begin() + size();
		}
		constexpr auto end() noexcept
		{
			return begin() + size();
		}
		constexpr auto cbegin() const noexcept
		{
			return sequence.cbegin();
		}
		constexpr auto cend() const noexcept
		{
			return cbegin() + size();
		}
		constexpr explicit operator bool() const noexcept
		{
			return error() == error_type::success;
		}
		friend encode_result encode(code_point_t) noexcept;
	private:
		code_unit_sequence_type sequence{}; // code point sequence
		size_type sequence_size{}; // size of code point sequence
		error_type error_code{ error_type::success };
	};

	inline encode_result encode(code_point_t character) noexcept
	{
		encode_result result{};

		if (is_scalar_value(character))
		{
			if (in_BMP(character))
			{
				result.sequence[0] = static_cast<code_unit_t>(character);
				result.sequence_size = 1;
			}
			else
			{
				character -= code_point_t(0x10000);
				result.sequence[1] = static_cast<code_unit_t>((character & code_point_t(0x03FF)) | min_trailing_surrogate);
				result.sequence[0] = static_cast<code_unit_t>((character >> 10) | min_leading_surrogate);
				result.sequence_size = 2;
			}
		}
		else
		{
			result.error_code = encode_error_code::not_scalar_value;
		}
		return result;
	}
}

#endif
