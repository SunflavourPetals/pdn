#ifndef PDN_Header_pdn_type_generator_std
#define PDN_Header_pdn_type_generator_std

#include <unordered_map>

#include "pdn_types.h"
#include "pdn_type_code.h"
#include "pdn_unicode_base.h"
#include "pdn_type_generator.h"

namespace pdn::dev_util
{
	template <typename>
	struct cppint_to_type_code
	{
		static_assert(false, "[pdn] \"cppint\" is not one pdn i types");
	};
	template <>
	struct cppint_to_type_code<types::i8>
	{
		static constexpr auto value = type_code::i8;
	};
	template <>
	struct cppint_to_type_code<types::i16>
	{
		static constexpr auto value = type_code::i16;
	};
	template <>
	struct cppint_to_type_code<types::i32>
	{
		static constexpr auto value = type_code::i32;
	};
	template <>
	struct cppint_to_type_code<types::i64>
	{
		static constexpr auto value = type_code::i64;
	};
	template <typename cppint_t>
	inline constexpr auto cppint_to_type_code_v = cppint_to_type_code<cppint_t>::value;

	template <typename>
	struct cppuint_to_type_code
	{
		static_assert(false, "[pdn] \"cppuint\" is not one pdn u types");
	};
	template <>
	struct cppuint_to_type_code<types::u8>
	{
		static constexpr auto value = type_code::u8;
	};
	template <>
	struct cppuint_to_type_code<types::u16>
	{
		static constexpr auto value = type_code::u16;
	};
	template <>
	struct cppuint_to_type_code<types::u32>
	{
		static constexpr auto value = type_code::u32;
	};
	template <>
	struct cppuint_to_type_code<types::u64>
	{
		static constexpr auto value = type_code::u64;
	};
	template <typename cppuint_t>
	inline constexpr auto cppuint_to_type_code_v = cppuint_to_type_code<cppuint_t>::value;
}

namespace pdn
{
#define PDN_Macro_Temp_make_map_for_typename_to_typecode(prefix) \
	self[prefix##"i8"_ucus]        = i8;        \
	self[prefix##"i16"_ucus]       = i16;       \
	self[prefix##"i32"_ucus]       = i32;       \
	self[prefix##"i64"_ucus]       = i64;       \
	self[prefix##"u8"_ucus]        = u8;        \
	self[prefix##"u16"_ucus]       = u16;       \
	self[prefix##"u32"_ucus]       = u32;       \
	self[prefix##"u64"_ucus]       = u64;       \
	self[prefix##"f32"_ucus]       = f32;       \
	self[prefix##"f64"_ucus]       = f64;       \
	self[prefix##"f"_ucus]         = f32;       \
	self[prefix##"float"_ucus]     = f32;       \
	self[prefix##"double"_ucus]    = f64;       \
	self[prefix##"boolean"_ucus]   = boolean;   \
	self[prefix##"bool"_ucus]      = boolean;   \
	self[prefix##"character"_ucus] = character; \
	self[prefix##"char"_ucus]      = character; \
	self[prefix##"c"_ucus]         = character; \
	self[prefix##"string"_ucus]    = string;    \
	self[prefix##"str"_ucus]       = string;    \
	self[prefix##"s"_ucus]         = string;    \
	self[prefix##"list"_ucus]      = list;      \
	self[prefix##"object"_ucus]    = object;    \
	self[prefix##"obj"_ucus]       = object;    \
	self[prefix##"cppint"_ucus]    = dev_util::cppint_to_type_code_v<types::cppint>;   \
	self[prefix##"int"_ucus]       = dev_util::cppint_to_type_code_v<types::cppint>;   \
	self[prefix##"i"_ucus]         = dev_util::cppint_to_type_code_v<types::cppint>;   \
	self[prefix##"cppuint"_ucus]   = dev_util::cppuint_to_type_code_v<types::cppuint>; \
	self[prefix##"uint"_ucus]      = dev_util::cppuint_to_type_code_v<types::cppuint>; \
	self[prefix##"u"_ucus]         = dev_util::cppuint_to_type_code_v<types::cppuint>;

	template <typename char_t>
	class type_table
	{
		static_assert(false, "[pdn] cannot generate type_table for template parameter \"char_t\"");
	};
	template <>
	class type_table<unicode::utf_8_code_unit_t> : public ::std::unordered_map<types::string<unicode::utf_8_code_unit_t>, type_code>
	{
	public:
		type_table()
		{
			using namespace literals::unicode_literals;
			using enum type_code;
			auto& self = *this;
			PDN_Macro_Temp_make_map_for_typename_to_typecode(u8);
		}
	};
	template <>
	class type_table<unicode::utf_16_code_unit_t> : public ::std::unordered_map<types::string<unicode::utf_16_code_unit_t>, type_code>
	{
	public:
		type_table()
		{
			using namespace literals::unicode_literals;
			using enum type_code;
			auto& self = *this;
			PDN_Macro_Temp_make_map_for_typename_to_typecode(u);
		}
	};
	template <>
	class type_table<unicode::utf_32_code_unit_t> : public ::std::unordered_map<types::string<unicode::utf_32_code_unit_t>, type_code>
	{
	public:
		type_table()
		{
			using namespace literals::unicode_literals;
			using enum type_code;
			auto& self = *this;
			PDN_Macro_Temp_make_map_for_typename_to_typecode(U);
		}
	};

#undef PDN_Macro_Temp_make_map_for_typename_to_typecode

	template <typename char_t>
	type_code type_generator_function(const types::string<char_t>& s)
	{
		static type_table<char_t> table{};
		if (auto result = table.find(s); result != table.end())
		{
			return result->second;
		}
		return type_code::unknown;
	}
}

#endif
