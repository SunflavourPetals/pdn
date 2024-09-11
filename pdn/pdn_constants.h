#ifndef PDN_Header_pdn_constants
#define PDN_Header_pdn_constants

#include <variant>
#include <functional>

#include "pdn_unicode_base.h"
#include "pdn_types.h"
#include "pdn_token_value_variant.h"

namespace pdn
{
	template <typename char_t>
	using constant_variant = ::std::variant<
		types::i8,  types::i16, types::i32, types::i64,
		types::u8,  types::u16, types::u32, types::u64,
		types::f32, types::f64,
		types::boolean,
		types::character<char_t>,
		types::string<char_t>>;
	template <typename char_t>
	using constants_generator = ::std::function<bool(const unicode::utf_8_code_unit_string&, constant_variant<char_t>&)>;
}

#endif
