#ifndef PDN_Header_pdn_error_message_generator_en
#define PDN_Header_pdn_error_message_generator_en

#include <type_traits>
#include <concepts>
#include <variant>

#include "pdn_error_string.h"
#include "pdn_error_code_variant.h"
#include "pdn_raw_error_message.h"
#include "pdn_err_msg_gen_en_lex.h"
#include "pdn_err_msg_gen_en_syn.h"
#include "pdn_err_msg_gen_en_utf8_dec.h"
#include "pdn_err_msg_gen_en_utf16_dec.h"
#include "pdn_err_msg_gen_en_utf32_dec.h"

namespace pdn
{
	inline auto error_message_generator_en(raw_error_message src) -> error_msg_string
	{
		return ::std::visit([&](auto code) -> error_msg_string
		{
			return detail::err_msg_gen_en(code, src.position, src.raw_error_message);
		}, src.error_code);
	}
}

#endif
