#ifndef PDN_Header_pdn_type
#define PDN_Header_pdn_type

#include "pdn_type_basic.h"
#include "pdn_type_config.h"
#include "pdn_unicode_base.h"
#include "pdn_entity_forward_decl.h"

namespace pdn::type
{
	using config::string;

	template <typename char_t>
	using list = config::list<entity<char_t>>;

	template <typename char_t>
	using object = config::object<string<char_t>, entity<char_t>>;

	using u8char  = character<unicode::u8char_t>;
	using u16char = character<unicode::u16char_t>;
	using u32char = character<unicode::u32char_t>;

	using u8string  = string<unicode::u8char_t>;
	using u16string = string<unicode::u16char_t>;
	using u32string = string<unicode::u32char_t>;

	using u8list  = list<unicode::u8char_t>;
	using u16list = list<unicode::u16char_t>;
	using u32list = list<unicode::u32char_t>;

	using u8object  = object<unicode::u8char_t>;
	using u16object = object<unicode::u16char_t>;
	using u32object = object<unicode::u32char_t>;
}

#endif
