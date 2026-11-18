#ifndef PDN_Header_pdn_entity_forward_decl
#define PDN_Header_pdn_entity_forward_decl

namespace pdn
{
	// Forward declarations only, so a header can name these types without including the definition.
	// That is what pdn_type.h needs: it uses entity<char_t> in its list/object aliases, while
	// pdn_entity.h includes pdn_type.h, so pulling the definition from pdn_type.h would cycle.
	template <typename char_t>
	class entity;

	template <typename char_t>
	class const_refer;

	template <typename char_t>
	class refer;
}

#endif
