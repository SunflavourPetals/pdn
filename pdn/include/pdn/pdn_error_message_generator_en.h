#ifndef PDN_Header_pdn_error_message_generator_en
#define PDN_Header_pdn_error_message_generator_en

#include <variant>

#include "pdn_error_string.h"
#include "pdn_raw_error_message.h"
#include "pdn_err_msg_gen_en_lex.h"
#include "pdn_err_msg_gen_en_syn.h"
#include "pdn_err_msg_gen_en_utf8_dec.h"
#include "pdn_err_msg_gen_en_utf16_dec.h"
#include "pdn_err_msg_gen_en_utf32_dec.h"

namespace pdn
{
	// English text for one raw error: dispatches on the active error code alternative to the matching
	// detail::err_msg_gen_en overload. The result is the detail only; the caller prefixes the category
	// name and the position (see default_error_message_generator).
	inline auto error_message_generator_en(raw_error_message src) -> error_msg_string
	{
		return ::std::visit([&](auto code) -> error_msg_string
		{
			return detail::err_msg_gen_en(code, src.position, src.raw_error_message);
		}, src.error_code);
	}
}

#endif
