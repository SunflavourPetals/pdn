#ifndef PDN_Header_pdn_error_message_generator_en
#define PDN_Header_pdn_error_message_generator_en

#include <variant>

#include "pdn_error_string.h"
#include "pdn_error_code_variant.h"
#include "pdn_raw_error_message_variant.h"
#include "pdn_err_msg_gen_en_lex.h"
#include "pdn_err_msg_gen_en_syn.h"
#include "pdn_err_msg_gen_en_utf_8_enc.h"
#include "pdn_err_msg_gen_en_utf_8_dec.h"
#include "pdn_err_msg_gen_en_utf_16_enc.h"
#include "pdn_err_msg_gen_en_utf_16_dec.h"
#include "pdn_err_msg_gen_en_utf_32_enc.h"
#include "pdn_err_msg_gen_en_utf_32_dec.h"

namespace pdn
{
	inline constexpr error_msg_string error_message_generator_en_function(error_code_variant errc, raw_error_message_variant src)
	{
		using namespace literals::error_message_literals;

		return ::std::visit([&](auto code) -> error_msg_string
		{
			if constexpr (requires { dev_util::err_msg_gen_en(code, error_msg_string{}); }) // todo
			{
				return dev_util::err_msg_gen_en(code, error_msg_string{});
			}
			return u8"unknown error type, error_message_generator_en unresolved"_em;
		}, errc);
	}
}

#endif
