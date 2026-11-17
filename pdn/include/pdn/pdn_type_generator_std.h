#ifndef PDN_Header_pdn_type_generator_std
#define PDN_Header_pdn_type_generator_std

#include <unordered_map>
#include <algorithm>

#include "pdn_type.h"
#include "pdn_false.h"
#include "pdn_type_code.h"
#include "pdn_unicode_base.h"

namespace pdn::detail
{
	template <typename T>
	struct cppint_to_type_code
	{
		static_assert(detail::false_v<T>, "[pdn] \"cppint\" is not one pdn i type");
	};
	template <>
	struct cppint_to_type_code<type::i8>
	{
		static constexpr auto value = type_code::i8;
	};
	template <>
	struct cppint_to_type_code<type::i16>
	{
		static constexpr auto value = type_code::i16;
	};
	template <>
	struct cppint_to_type_code<type::i32>
	{
		static constexpr auto value = type_code::i32;
	};
	template <>
	struct cppint_to_type_code<type::i64>
	{
		static constexpr auto value = type_code::i64;
	};
	template <typename cppint_t>
	constexpr auto cppint_to_type_code_v = cppint_to_type_code<cppint_t>::value;

	template <typename T>
	struct cppuint_to_type_code
	{
		static_assert(detail::false_v<T>, "[pdn] \"cppuint\" is not one pdn u type");
	};
	template <>
	struct cppuint_to_type_code<type::u8>
	{
		static constexpr auto value = type_code::u8;
	};
	template <>
	struct cppuint_to_type_code<type::u16>
	{
		static constexpr auto value = type_code::u16;
	};
	template <>
	struct cppuint_to_type_code<type::u32>
	{
		static constexpr auto value = type_code::u32;
	};
	template <>
	struct cppuint_to_type_code<type::u64>
	{
		static constexpr auto value = type_code::u64;
	};
	template <typename cppuint_t>
	constexpr auto cppuint_to_type_code_v = cppuint_to_type_code<cppuint_t>::value;
}

namespace pdn
{
	// The type names and aliases listed by docs/pdn-reference.md (the section that enumerates the
	// standard aliases), plus two non-standard spellings, cppint and cppuint, which docs/pdn-guide.md
	// notes as accepted by this implementation only. int/i and uint/u resolve through
	// type::auto_int and type::auto_uint, i.e. to whichever fixed-width type C++ int/unsigned int
	// is, with i32/u32 as the fallback. The u16/u32 tables are derived from this one, which is
	// safe because every type name is ASCII.
	template <typename char_t>
	class type_table
	{
		static_assert(detail::false_v<char_t>, "[pdn] cannot generate type_table for template parameter \"char_t\"");
	};
	template <>
	class type_table<unicode::u8char_t> : public ::std::unordered_map<type::string<unicode::u8char_t>, type_code>
	{
	public:
		type_table()
		{
			using namespace literals::unicode_literals;
			using enum type_code;
			auto& self = *this;
			constexpr auto pl_int = detail::cppint_to_type_code_v<type::auto_int>;
			constexpr auto pl_uint = detail::cppuint_to_type_code_v<type::auto_uint>;
			self[u8"i8"_s]        = i8;
			self[u8"i16"_s]       = i16;
			self[u8"i32"_s]       = i32;
			self[u8"i64"_s]       = i64;
			self[u8"u8"_s]        = u8;
			self[u8"u16"_s]       = u16;
			self[u8"u32"_s]       = u32;
			self[u8"u64"_s]       = u64;
			self[u8"f32"_s]       = f32;
			self[u8"f64"_s]       = f64;
			self[u8"f"_s]         = f32;
			self[u8"float"_s]     = f32;
			self[u8"double"_s]    = f64;
			self[u8"boolean"_s]   = boolean;
			self[u8"bool"_s]      = boolean;
			self[u8"character"_s] = character;
			self[u8"char"_s]      = character;
			self[u8"c"_s]         = character;
			self[u8"string"_s]    = string;
			self[u8"str"_s]       = string;
			self[u8"s"_s]         = string;
			self[u8"list"_s]      = list;
			self[u8"object"_s]    = object;
			self[u8"obj"_s]       = object;
			self[u8"int"_s]       = pl_int;
			self[u8"i"_s]         = pl_int;
			self[u8"uint"_s]      = pl_uint;
			self[u8"u"_s]         = pl_uint;
#ifndef PDN_NO_EXTENSIONS
			self[u8"cppint"_s]    = pl_int;  // non-standard type name
			self[u8"cppuint"_s]   = pl_uint; // non-standard type name
#endif
		}
		static auto instance() -> const type_table&
		{
			static auto table = type_table{};
			return table;
		}
	};
	template <>
	class type_table<unicode::u16char_t> : public ::std::unordered_map<type::string<unicode::u16char_t>, type_code>
	{
	public:
		type_table()
		{
			const auto& u8instance = type_table<unicode::u8char_t>::instance();
			auto& self = *this;
			for (const auto& pair : u8instance)
			{
				auto key = type::string<unicode::u16char_t>(pair.first.size(), unicode::u16char_t{});
				// standard has only ascii type name
				::std::copy(::std::cbegin(pair.first), ::std::cend(pair.first), key.begin());
				self[::std::move(key)] = pair.second;
			}
		}
		static auto instance() -> const type_table&
		{
			static auto table = type_table{};
			return table;
		}
	};
	template <>
	class type_table<unicode::u32char_t> : public ::std::unordered_map<type::string<unicode::u32char_t>, type_code>
	{
	public:
		type_table()
		{
			const auto& u8instance = type_table<unicode::u8char_t>::instance();
			auto& self = *this;
			for (const auto& pair : u8instance)
			{
				auto key = type::string<unicode::u32char_t>(pair.first.size(), unicode::u32char_t{});
				// standard has only ascii type name
				::std::copy(::std::cbegin(pair.first), ::std::cend(pair.first), key.begin());
				self[::std::move(key)] = pair.second;
			}
		}
		static auto instance() -> const type_table&
		{
			static auto table = type_table{};
			return table;
		}
	};

	template <typename char_t>
	type_code type_generator_function(const type::string<char_t>& s)
	{
		const auto& table = type_table<char_t>::instance();
		if (auto result = table.find(s); result != table.end())
		{
			return result->second;
		}
		return type_code::unknown;
	}
}

#endif
