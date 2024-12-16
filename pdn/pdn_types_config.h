#ifndef PDN_Header_pdn_types_config
#define PDN_Header_pdn_types_config

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

#include "pdn_types_character.h"

namespace pdn::types::config
{
	using i8  = ::std::int8_t;
	using i16 = ::std::int16_t;
	using i32 = ::std::int32_t;
	using i64 = ::std::int64_t;
	
	using u8  = ::std::uint8_t;
	using u16 = ::std::uint16_t;
	using u32 = ::std::uint32_t;
	using u64 = ::std::uint64_t;
	
	using f32 = float;
	using f64 = double;

	using boolean = bool;

	template <typename char_t>
	using character = impl::character<char_t>;

	template <typename char_t>
	using string = ::std::basic_string<char_t>;
	
	template <typename entity_t>
	using list = ::std::vector<entity_t>;
	
	template <typename iden_t, typename entity_t>
	using object = ::std::unordered_map<iden_t, entity_t>;
}

#endif
