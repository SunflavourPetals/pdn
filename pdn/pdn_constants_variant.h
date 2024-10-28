#ifndef PDN_Header_pdn_constants_variant
#define PDN_Header_pdn_constants_variant

#include <variant>

#include "pdn_types.h"

namespace pdn
{
	template <typename char_t>
	using constant_variant = ::std::variant<
		types::i8, types::i16, types::i32, types::i64,
		types::u8, types::u16, types::u32, types::u64,
		types::f32, types::f64,
		types::boolean,
		types::character<char_t>,
		types::string<char_t>>;
}

#endif
