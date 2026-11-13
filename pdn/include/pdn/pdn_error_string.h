#ifndef PDN_Header_pdn_error_string
#define PDN_Header_pdn_error_string

#include <cstddef>
#include <string>
#include <string_view>

namespace pdn
{
	// Error text is always UTF-8, whatever character type was used to parse the input: the parsed
	// data keeps its own type, but the lexer/parser pass their error details to the message
	// generators as this type, so one message vocabulary serves char8_t, char16_t and char32_t.
	using error_msg_char = char8_t;
	using error_msg_string = ::std::basic_string<error_msg_char>;
	using error_msg_string_view = ::std::basic_string_view<error_msg_char>;

	// Copies the bytes as they are; no transcoding happens here, so the input must already be
	// UTF-8. That holds for ASCII digits from std::to_string and for the fragments the lexer
	// builds, which are produced in this encoding upstream.
	inline auto reinterpret_to_err_msg_str(const char* begin, const char* end) -> error_msg_string
	{
		return { reinterpret_cast<const error_msg_char*>(begin), reinterpret_cast<const error_msg_char*>(end) };
	}

	inline auto reinterpret_to_err_msg_str(const ::std::string_view s) -> error_msg_string
	{
		return reinterpret_to_err_msg_str(s.data(), s.data() + s.size());
	}
}

namespace pdn::inline literals::inline error_message_literals
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

#endif
