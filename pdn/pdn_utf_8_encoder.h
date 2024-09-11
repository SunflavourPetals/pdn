#ifndef PDN_Header_pdn_utf_8_encoder
#define PDN_Header_pdn_utf_8_encoder

#include <array>
#include <utility>

#include "pdn_unicode_base.h"
#include "pdn_utf_8_base.h"

namespace pdn::unicode::utf_8
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
		using code_unit_sequence_type = ::std::array<code_unit_t, 4>;
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
		friend encode_result encode(code_point_t character) noexcept;
	private:
		code_unit_sequence_type sequence{}; // code point sequence
		size_type sequence_size{}; // size of code point sequence
		error_type error_code{ error_type::success };
	};
}

namespace pdn::unicode::utf_8
{
	inline encode_result encode(code_point_t character) noexcept
	{
		encode_result result{};

		if (!is_scalar_value(character))
		{
			result.error_code = encode_error_code::not_scalar_value;
			return result;
		}

		if (character < code_point_t(0x80)) // U+0000..U+007F | 1 code unit
		{
			result.sequence[0] = static_cast<code_unit_t>(character);
			result.sequence_size = 1;
		}
		else if (character < code_point_t(0x0800)) // U+0080..U+07FF | 2 code units
		{
			result.sequence[1] = (code_unit_t(0x80) | (character & code_unit_t(0x3F))); // low 6 bits
			result.sequence[0] = (code_unit_t(0xC0) | (character >> 6)); // high 5 bits
			result.sequence_size = 2;
		}
		else if (in_BMP(character)) // U+0800..U+FFFF | 3 code units
		{
			result.sequence[2] = (code_unit_t(0x80) | (character & code_unit_t(0x3F))); // low 6 bits
			result.sequence[1] = (code_unit_t(0x80) | ((character >> 6) & code_unit_t(0x3F))); // low 7..12 bits
			result.sequence[0] = (code_unit_t(0xE0) | (character >> 12)); // high 4 bits
			result.sequence_size = 3;
		}
		else // passed is_scalar_value -> this branch means U+10000..U+10FFFF | 4 code units
		{
			result.sequence[3] = (code_unit_t(0x80) | (character & code_unit_t(0x3F))); // low 6 bits
			result.sequence[2] = (code_unit_t(0x80) | ((character >> 6) & code_unit_t(0x3F))); // low 7..12 bits
			result.sequence[1] = (code_unit_t(0x80) | ((character >> 12) & code_unit_t(0x3F))); // low 13..18 bits
			result.sequence[0] = (code_unit_t(0xF0) | (character >> 18)); // high 3 bits
			result.sequence_size = 4;
		}

		return result;
	}
}

#endif
