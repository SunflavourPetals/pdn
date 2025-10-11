#ifndef PDN_Header_pdn_utf_encoder
#define PDN_Header_pdn_utf_encoder

#include "pdn_unicode_base.h"
#include "pdn_utf_8_base.h"
#include "pdn_utf_8_encoder.h"
#include "pdn_utf_16_base.h"
#include "pdn_utf_16_encoder.h"
#include "pdn_utf_32_base.h"
#include "pdn_utf_32_encoder.h"

namespace pdn::unicode::detail
{
	template <typename char_t>
	struct encoder {};
	template <>
	struct encoder<u8char_t>
	{
		using result = utf_8::encode_result;
		static auto encode(code_point_t c) -> result { return utf_8::encode(c); }
	};
	template <>
	struct encoder<u16char_t>
	{
		using result = utf_16::encode_result;
		static auto encode(code_point_t c) -> result { return utf_16::encode(c); }
	};
	template <>
	struct encoder<u32char_t>
	{
		using result = utf_32::encode_result;
		static auto encode(code_point_t c) -> result { return utf_32::encode(c); }
	};
	template <typename char_t>
	using encode_result_t = encoder<char_t>::result;
}

namespace pdn::unicode
{
	inline auto encode(code_point_t character, u8char_t) -> utf_8::encode_result
	{
		return utf_8::encode(character);
	}
	inline auto encode(code_point_t character, u16char_t) -> utf_16::encode_result
	{
		return utf_16::encode(character);
	}
	inline auto encode(code_point_t character, u32char_t) -> utf_32::encode_result
	{
		return utf_32::encode(character);
	}
	template <typename char_t>
	inline auto encode(code_point_t character) -> detail::encode_result_t<char_t>
	{
		return detail::encoder<char_t>::encode(character);
	}
}

namespace pdn::unicode::detail
{
	template <typename char_t> struct suitable_encoder {};
	template <> struct suitable_encoder<u8char_t>  { using type = utf_8::encoder; };
	template <> struct suitable_encoder<u16char_t> { using type = utf_16::encoder; };
	template <> struct suitable_encoder<u32char_t> { using type = utf_32::encoder; };

	template <typename char_t>
	using suitable_encoder_t = suitable_encoder<char_t>::type;
}

namespace pdn::unicode
{
	template <concepts::code_unit char_t>
	using encoder = detail::suitable_encoder_t<char_t>;
}

#endif
