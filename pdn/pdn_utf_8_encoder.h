#ifndef PDN_Header_pdn_utf_8_encoder
#define PDN_Header_pdn_utf_8_encoder

#include <array>
#include <utility>
#include <cstdint>

#include "pdn_unicode_base.h"
#include "pdn_utf_8_base.h"

namespace pdn::unicode::utf_8
{
	enum class encode_error_code : ::std::uint16_t
	{
		not_scalar_value = 1,
	};

	class encoder;

	class [[nodiscard("encode_result should be processed")]] encode_result final
	{
	public:
		using code_unit_sequence_type = ::std::array<code_unit_t, 4>;
		using size_type               = ::std::uint16_t;
		using error_type              = encode_error_code;
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
		constexpr auto failed() const noexcept
		{
			return error_code != error_type{};
		}
		constexpr explicit operator bool() const noexcept
		{
			return !failed();
		}
		friend class encoder;
	private:
		code_unit_sequence_type sequence{};
		size_type               sequence_size{};
		error_type              error_code{};
	};

	class encoder
	{
	public:
		static auto encode(code_point_t character) noexcept -> encode_result
		{
			encode_result result{};

			if (!is_scalar_value(character))
			{
				result.error_code = encode_error_code::not_scalar_value;
				return result;
			}

			using cp_t = code_point_t;
			using cu_t = code_unit_t;

			if (character < cp_t(0x80u)) // U+0000..U+007F | 1 code unit
			{
				result.sequence[0] = static_cast<cu_t>(character);
				result.sequence_size = 1;
			}
			else if (character < cp_t(0x0800u)) // U+0080..U+07FF | 2 code units
			{
				result.sequence[1] = static_cast<cu_t>(cp_t(0x80u) | ( character         & cp_t(0x3Fu))); // low  6 bits
				result.sequence[0] = static_cast<cu_t>(cp_t(0xC0u) |  (character >> 6u)                ); // high 5 bits
				result.sequence_size = 2;
			}
			else if (is_in_BMP(character)) // U+0800..U+FFFF | 3 code units
			{
				result.sequence[2] = static_cast<cu_t>(cp_t(0x80u) | ( character         & cp_t(0x3Fu))); // low  6 bits
				result.sequence[1] = static_cast<cu_t>(cp_t(0x80u) | ((character >> 6u)  & cp_t(0x3Fu))); // low  7..12 bits
				result.sequence[0] = static_cast<cu_t>(cp_t(0xE0u) |  (character >> 12u)               ); // high 4 bits
				result.sequence_size = 3;
			}
			else // passed is_scalar_value -> this branch means U+10000..U+10FFFF | 4 code units
			{
				result.sequence[3] = static_cast<cu_t>(cp_t(0x80u) | ( character         & cp_t(0x3Fu))); // low  6 bits
				result.sequence[2] = static_cast<cu_t>(cp_t(0x80u) | ((character >> 6u)  & cp_t(0x3Fu))); // low  7..12 bits
				result.sequence[1] = static_cast<cu_t>(cp_t(0x80u) | ((character >> 12u) & cp_t(0x3Fu))); // low  13..18 bits
				result.sequence[0] = static_cast<cu_t>(cp_t(0xF0u) |  (character >> 18u)               ); // high 3 bits
				result.sequence_size = 4;
			}

			return result;
		}
	};

	inline auto encode(code_point_t character) noexcept -> encode_result
	{
		return encoder::encode(character);
	}
}

#endif
