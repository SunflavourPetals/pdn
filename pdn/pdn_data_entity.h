#ifndef PDN_Header_pdn_data_entity
#define PDN_Header_pdn_data_entity

#include "pdn_types.h"
#include "pdn_entity.h"
#include "pdn_entity_as_accessor.h"
#include "pdn_entity_get_accessor.h"

namespace pdn
{
	template <typename char_t>
	using data_entity = entity<char_t>;
}

#endif
