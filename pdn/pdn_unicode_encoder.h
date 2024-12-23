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
	struct encode_result {};
	template <>
	struct encode_result<utf_8_code_unit_t>
	{
		using type = utf_8::encode_result;
	};
	template <>
	struct encode_result<utf_16_code_unit_t>
	{
		using type = utf_16::encode_result;
	};
	template <>
	struct encode_result<utf_32_code_unit_t>
	{
		using type = utf_32::encode_result;
	};
	template <typename char_t>
	using encode_result_t = encode_result<char_t>::type;
}

namespace pdn::unicode
{
	inline auto encode(const code_point_t character, utf_8_code_unit_t) -> utf_8::encode_result
	{
		return utf_8::encode(character);
	}
	inline auto encode(const code_point_t character, utf_16_code_unit_t) -> utf_16::encode_result
	{
		return utf_16::encode(character);
	}
	inline auto encode(const code_point_t character, utf_32_code_unit_t) -> utf_32::encode_result
	{
		return utf_32::encode(character);
	}
	template <typename char_t>
	inline auto encode(const code_point_t character) -> dev_util::encode_result_t<char_t>
	{
		return encode(character, char_t{});
	}

	class encoder
	{
	public:
		template <typename char_t>
		static auto decode(const code_point_t character)
		{
			return ::pdn::unicode::encode<char_t>(character);
		}
	};
}

#endif
