#ifndef PDN_Header_pdn_types
#define PDN_Header_pdn_types

#include "pdn_types_basic.h"
#include "pdn_types_config.h"
#include "pdn_entity_forward_decl.h"

namespace pdn::types
{
	using config::string;

	template <typename char_t>
	using list = config::list<entity<char_t>>;

	template <typename char_t>
	using object = config::object<string<char_t>, entity<char_t>>;
}

#endif
