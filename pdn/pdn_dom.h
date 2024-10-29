#ifndef PDN_Header_pdn_dom
#define PDN_Header_pdn_dom

#include "pdn_types.h"

namespace pdn
{
	template <typename char_t>
	using document_object_model = types::entity<char_t>;

	template <typename char_t>
	using dom = document_object_model<char_t>;
}

#endif
