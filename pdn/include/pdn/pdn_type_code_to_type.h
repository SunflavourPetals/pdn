#ifndef PDN_Header_pdn_type_code_to_type
#define PDN_Header_pdn_type_code_to_type

#include "pdn_type.h"
#include "pdn_type_code.h"

namespace pdn
{
	template <type_code type_c, typename char_t>
	struct type_code_to_type
	{
		using type = void;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::i8, char_t>
	{
		using type = type::i8;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::i16, char_t>
	{
		using type = type::i16;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::i32, char_t>
	{
		using type = type::i32;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::i64, char_t>
	{
		using type = type::i64;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::u8, char_t>
	{
		using type = type::u8;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::u16, char_t>
	{
		using type = type::u16;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::u32, char_t>
	{
		using type = type::u32;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::u64, char_t>
	{
		using type = type::u64;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::f32, char_t>
	{
		using type = type::f32;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::f64, char_t>
	{
		using type = type::f64;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::boolean, char_t>
	{
		using type = type::boolean;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::character, char_t>
	{
		using type = type::character<char_t>;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::string, char_t>
	{
		using type = type::string<char_t>;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::list, char_t>
	{
		using type = type::list<char_t>;
	};
	template <typename char_t>
	struct type_code_to_type<type_code::object, char_t>
	{
		using type = type::object<char_t>;
	};
	template <type_code type_c, typename char_t>
	using type_code_to_type_t = typename type_code_to_type<type_c, char_t>::type;
}

#endif
