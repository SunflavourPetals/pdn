#ifndef PDN_Header_pdn_utf_32_encoder
#define PDN_Header_pdn_utf_32_encoder

#include <array>
#include <cstdint>

#include "pdn_unicode_base.h"
#include "pdn_utf_32_base.h"

namespace pdn::unicode::utf_32
{
	enum class encode_error_code : ::std::uint16_t
	{
		not_scalar_value = 1,
	};

	class encoder;

	class [[nodiscard("encode_result should be processed")]] encode_result final
	{
	public:
		using code_unit_sequence_type = ::std::array<code_unit_t, 1>;
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
			if (is_scalar_value(character)) [[likely]]
			{
				result.sequence[0] = character;
				result.sequence_size = 1;
			}
			else
			{
				result.error_code = encode_error_code::not_scalar_value;
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
