#ifndef PDN_Header_pdn_dom
#define PDN_Header_pdn_dom

#include "pdn_types.h"
#include "pdn_entity.h"
#include "pdn_entity_as_accessor.h"
#include "pdn_entity_get_accessor.h"

namespace pdn
{
	template <typename char_t>
	using data_entity = entity<char_t>;

	template <typename char_t>
	using document_object_model = entity<char_t>;

	template <typename char_t>
	using dom = document_object_model<char_t>;
}

#endif
