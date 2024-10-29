#ifndef PDN_Header_pdn_error_string
#define PDN_Header_pdn_error_string

#include <string>

namespace pdn
{
	using error_msg_char        = char8_t;
	using error_msg_string      = ::std::basic_string<error_msg_char>;
	using error_msg_string_view = ::std::basic_string_view<error_msg_char>;

	inline auto reinterpret_to_err_msg_str(const char* begin, const char* end) -> error_msg_string
	{
		return { reinterpret_cast<const error_msg_char*>(begin), reinterpret_cast<const error_msg_char*>(end) };
	}

	inline auto reinterpret_to_err_msg_str(const ::std::string_view s) -> error_msg_string
	{
		return reinterpret_to_err_msg_str(s.data(), s.data() + s.size());
	}

	inline namespace literals
	{
		inline namespace error_message_literals
		{
			[[nodiscard]] constexpr error_msg_string operator""_em(const error_msg_char* ptr, ::std::size_t length)
			{
				return error_msg_string{ ptr, length };
			}

			[[nodiscard]] constexpr error_msg_string_view operator""_emv(const error_msg_char* ptr, ::std::size_t length)
			{
				return error_msg_string_view{ ptr, length };
			}
		}
	}
}

#endif
