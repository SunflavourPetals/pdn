#ifndef PDN_Header_pdn_unicode_encoder
#define PDN_Header_pdn_unicode_encoder

#include "pdn_unicode_base.h"
#include "pdn_utf_8_base.h"
#include "pdn_utf_8_encoder.h"
#include "pdn_utf_16_base.h"
#include "pdn_utf_16_encoder.h"
#include "pdn_utf_32_base.h"
#include "pdn_utf_32_encoder.h"

namespace pdn::unicode::dev_util
{
	template <typename char_t>
	struct encoder {};
	template <>
	struct encoder<utf_8_code_unit_t>
	{
		using result = utf_8::encode_result;
		static auto encode(code_point_t c) -> result { return utf_8::encode(c); }
	};
	template <>
	struct encoder<utf_16_code_unit_t>
	{
		using result = utf_16::encode_result;
		static auto encode(code_point_t c) -> result { return utf_16::encode(c); }
	};
	template <>
	struct encoder<utf_32_code_unit_t>
	{
		using result = utf_32::encode_result;
		static auto encode(code_point_t c) -> result { return utf_32::encode(c); }
	};
	template <typename char_t>
	using encode_result_t = encoder<char_t>::result;
}

namespace pdn::unicode
{
	inline auto encode(code_point_t character, utf_8_code_unit_t) -> utf_8::encode_result
	{
		return utf_8::encode(character);
	}
	inline auto encode(code_point_t character, utf_16_code_unit_t) -> utf_16::encode_result
	{
		return utf_16::encode(character);
	}
	inline auto encode(code_point_t character, utf_32_code_unit_t) -> utf_32::encode_result
	{
		return utf_32::encode(character);
	}
	template <typename char_t>
	inline auto encode(code_point_t character) -> dev_util::encode_result_t<char_t>
	{
		return dev_util::encoder<char_t>::encode(character);
	}

	class encoder
	{
	public:
		template <typename char_t>
		static auto encode(code_point_t character)
		{
			return dev_util::encoder<char_t>::encode(character);
		}
	};
}

#endif
