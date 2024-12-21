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
#include "pdn_err_msg_gen_en_utf_8_enc.h"
#include "pdn_err_msg_gen_en_utf_8_dec.h"
#include "pdn_err_msg_gen_en_utf_16_enc.h"
#include "pdn_err_msg_gen_en_utf_16_dec.h"
#include "pdn_err_msg_gen_en_utf_32_enc.h"
#include "pdn_err_msg_gen_en_utf_32_dec.h"

namespace pdn
{
	inline auto error_message_generator_en(raw_error_message src) -> error_msg_string
	{
		return ::std::visit([&](auto code) -> error_msg_string
		{
			return dev_util::err_msg_gen_en(code, src.position, src.raw_error_message);
			using namespace literals::error_message_literals;
			return u8"unknown error type, error_message_generator_en unresolved"_em;
		}, src.error_code);
	}
}

#endif
