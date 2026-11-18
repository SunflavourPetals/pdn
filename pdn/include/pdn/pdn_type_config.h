#ifndef PDN_Header_pdn_type_config
#define PDN_Header_pdn_type_config

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

#include "pdn_unicode_base.h"
#include "pdn_type_character.h"

namespace pdn::type::config
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
	using character = detail::character<char_t>;

	template <typename char_t>
	using string = ::std::basic_string<char_t>;
	
	template <typename entity_t>
	using list = ::std::vector<entity_t>;
	
	// Hashes identifier text without building a key object: is_transparent plus these three
	// overloads let find/count/contains take a string view of the object's character type.
	struct key_hasher
	{
		using is_transparent = void; // enables heterogeneous operations.
		using u8sv  = unicode::u8string_view;
		using u16sv = unicode::u16string_view;
		using u32sv = unicode::u32string_view;

		auto operator()(const u8sv sv) const -> ::std::size_t
		{
			return ::std::hash<u8sv>{}(sv);
		}
		auto operator()(const u16sv sv) const -> ::std::size_t
		{
			return ::std::hash<u16sv>{}(sv);
		}
		auto operator()(const u32sv sv) const -> ::std::size_t
		{
			return ::std::hash<u32sv>{}(sv);
		}
	};

// Without PDN_USER_OBJECT this is an unordered_map, whose find/count/contains accept a
// string view thanks to key_hasher; at() converts to the key type, so lookups by view use find.
// With the macro it is PDN_USER_OBJECT<iden_t, entity_t>.
#ifdef PDN_USER_OBJECT
	template <typename iden_t, typename entity_t>
	using object = PDN_USER_OBJECT<iden_t, entity_t>;
#else
	template <typename iden_t, typename entity_t>
	using object = ::std::unordered_map<iden_t, entity_t, key_hasher, ::std::equal_to<>>;
#endif
}

#endif
