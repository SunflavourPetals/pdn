#ifndef PDN_Header_pdn_type_to_type_code
#define PDN_Header_pdn_type_to_type_code

#include "pdn_types.h"
#include "pdn_type_code.h"

namespace pdn
{
	template <typename unknown_type, typename char_t>
	struct type_to_type_code
	{
		static constexpr type_code value = type_code::unknown;
	};
	template <typename char_t>
	struct type_to_type_code<types::i8, char_t>
	{
		static constexpr type_code value = type_code::i8;
	};
	template <typename char_t>
	struct type_to_type_code<types::i16, char_t>
	{
		static constexpr type_code value = type_code::i16;
	};
	template <typename char_t>
	struct type_to_type_code<types::i32, char_t>
	{
		static constexpr type_code value = type_code::i32;
	};
	template <typename char_t>
	struct type_to_type_code<types::i64, char_t>
	{
		static constexpr type_code value = type_code::i64;
	};
	template <typename char_t>
	struct type_to_type_code<types::u8, char_t>
	{
		static constexpr type_code value = type_code::u8;
	};
	template <typename char_t>
	struct type_to_type_code<types::u16, char_t>
	{
		static constexpr type_code value = type_code::u16;
	};
	template <typename char_t>
	struct type_to_type_code<types::u32, char_t>
	{
		static constexpr type_code value = type_code::u32;
	};
	template <typename char_t>
	struct type_to_type_code<types::u64, char_t>
	{
		static constexpr type_code value = type_code::u64;
	};
	template <typename char_t>
	struct type_to_type_code<types::f32, char_t>
	{
		static constexpr type_code value = type_code::f32;
	};
	template <typename char_t>
	struct type_to_type_code<types::f64, char_t>
	{
		static constexpr type_code value = type_code::f64;
	};
	template <typename char_t>
	struct type_to_type_code<types::boolean, char_t>
	{
		static constexpr type_code value = type_code::boolean;
	};
	template <typename char_t>
	struct type_to_type_code<types::character<char_t>, char_t>
	{
		static constexpr type_code value = type_code::character;
	};
	template <typename char_t>
	struct type_to_type_code<types::string<char_t>, char_t>
	{
		static constexpr type_code value = type_code::string;
	};
	template <typename char_t>
	struct type_to_type_code<types::list<char_t>, char_t>
	{
		static constexpr type_code value = type_code::list;
	};
	template <typename char_t>
	struct type_to_type_code<types::object<char_t>, char_t>
	{
		static constexpr type_code value = type_code::object;
	};
	template <typename type, typename char_t>
	inline constexpr type_code type_to_type_code_v = type_to_type_code<type, char_t>::value;
}

#endif
