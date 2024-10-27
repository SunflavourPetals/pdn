#ifndef PDN_Header_pdn_error_handler_concept
#define PDN_Header_pdn_error_handler_concept

#include "pdn_error_message.h"

namespace pdn::concepts
{
	template <typename type>
	concept error_handler = requires (type err_handler, const error_message err_msg)
	{
		err_handler.handle_error(err_msg);
	};
}

#endif
