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

	template <typename char_t>
	using data_entity_ref = refer<char_t>;

	template <typename char_t>
	using data_entity_cref = const_refer<char_t>;

	using u8entity       = data_entity<char8_t>;
	using u8entity_ref   = data_entity_ref<char8_t>;
	using u8entity_cref  = data_entity_cref<char8_t>;
	using u16entity      = data_entity<char16_t>;
	using u16entity_ref  = data_entity_ref<char16_t>;
	using u16entity_cref = data_entity_cref<char16_t>;
	using u32entity      = data_entity<char32_t>;
	using u32entity_ref  = data_entity_ref<char32_t>;
	using u32entity_cref = data_entity_cref<char32_t>;
}

#endif
