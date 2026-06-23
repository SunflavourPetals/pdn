#ifndef PDN_Header_pdn_type
#define PDN_Header_pdn_type

#include "pdn_type_basic.h"
#include "pdn_type_config.h"
#include "pdn_entity_forward_decl.h"

namespace pdn::type
{
	using config::string;

	template <typename char_t>
	using list = config::list<entity<char_t>>;

	template <typename char_t>
	using object = config::object<string<char_t>, entity<char_t>>;
}

#endif
