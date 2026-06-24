#ifndef PDN_Header_pdn_entity_utility
#define PDN_Header_pdn_entity_utility

#include <type_traits>
#include <variant>
#include <cmath>

#include "pdn_type.h"
#include "pdn_proxy.h"

#include "pdn_entity_forward_decl.h"

namespace pdn::type::detail
{
	template <typename char_t>
	using entity_variant = ::std::variant<
		i32, i64, i8, i16,
		u32, u64, u8, u16,
		f32, f64,
		boolean,
		character<char_t>,
		proxy<string<char_t>>,
		proxy<list<char_t>>,
		proxy<object<char_t>>>;
}

namespace pdn::detail
{
	using as_int_default_t  = type::i64;
	using as_uint_default_t = type::u64;
	using as_fp_default_t   = type::f64;

	template <typename arg_t, type::concepts::pdn_sint tar_int = as_int_default_t>
	inline auto as_int(const arg_t& arg) -> tar_int
	{
		using namespace type::concepts;
		if constexpr (pdn_bool<arg_t>)
		{
			return arg;
		}
		else if constexpr (pdn_sint<arg_t>)
		{
			constexpr auto tar_max = ::std::numeric_limits<tar_int>::max();
			constexpr auto tar_min = ::std::numeric_limits<tar_int>::min();
			if (arg > tar_max) return tar_max;
			if (arg < tar_min) return tar_min;
			return static_cast<tar_int>(arg);
		}
		else if constexpr (pdn_uint<arg_t>)
		{
			constexpr auto max = ::std::numeric_limits<tar_int>::max();
			return arg > type::u64(max) ? max : tar_int(arg);
		}
		else if constexpr (pdn_fp<arg_t>)
		{
			constexpr auto max = ::std::numeric_limits<tar_int>::max();
			constexpr auto min = ::std::numeric_limits<tar_int>::min();
			if (::std::isnan(arg)) return 0;
			if (arg >= arg_t(max)) return max;
			if (arg <= arg_t(min)) return min;
			return static_cast<tar_int>(arg);
		}
		else
		{
			return 0;
		}
	}

	template <typename arg_t, type::concepts::pdn_uint tar_uint = as_uint_default_t>
	inline auto as_uint(const arg_t& arg) -> tar_uint
	{
		using namespace type::concepts;
		if constexpr (pdn_bool<arg_t>)
		{
			return arg;
		}
		else if constexpr (pdn_uint<arg_t>)
		{
			constexpr auto tar_max = ::std::numeric_limits<tar_uint>::max();
			if (arg > tar_max) return tar_max;
			return static_cast<tar_uint>(arg);
		}
		else if constexpr (pdn_sint<arg_t>)
		{
			constexpr auto tar_max = ::std::numeric_limits<tar_uint>::max();
			if (arg < 0) return 0;
			if (static_cast<type::u64>(arg) > tar_max) return tar_max;
			return static_cast<tar_uint>(arg);
		}
		else if constexpr (pdn_fp<arg_t>)
		{
			constexpr auto max = ::std::numeric_limits<tar_uint>::max();
			if (::std::isnan(arg)) return 0;
			if (arg > arg_t(max)) return max;
			return arg < 0 ? tar_uint(0) : tar_uint(arg);
		}
		else
		{
			return 0;
		}
	}

	template <typename arg_t, type::concepts::pdn_fp tar_fp = as_fp_default_t>
	inline auto as_fp(const arg_t& arg) -> tar_fp
	{
		using namespace type::concepts;
		if constexpr (pdn_bool<arg_t> || pdn_integral<arg_t> || pdn_fp<arg_t>)
		{
			return static_cast<tar_fp>(arg);
		}
		else
		{
			return 0;
		}
	}

	template <typename char_t, typename arg_t>
	inline auto as_bool(const arg_t& arg) -> type::boolean
	{
		using namespace type::concepts;
		if constexpr (pdn_bool<arg_t> || pdn_integral<arg_t> || pdn_fp<arg_t>)
		{
			return static_cast<bool>(arg);
		}
		else if constexpr (::std::same_as<arg_t, type::character<char_t>>)
		{
			return arg != type::character<char_t>{};
		}
		else
		{
			return false;
		}
	}

	template <typename char_t>
	struct as_accessor
	{
		template <typename arg_t, typename in = as_int_default_t>
		static auto as_int(const arg_t& arg) -> in
		{
			return detail::as_int<arg_t, in>(arg);
		}

		template <typename arg_t, typename un = as_uint_default_t>
		static auto as_uint(const arg_t& arg) -> un
		{
			return detail::as_uint<arg_t, un>(arg);
		}

		template <typename arg_t, typename fn = as_fp_default_t>
		static auto as_fp(const arg_t& arg) -> fn
		{
			return detail::as_fp<arg_t, fn>(arg);
		}

		template <typename arg_t>
		static auto as_bool(const arg_t& arg) -> type::boolean
		{
			return detail::as_bool<char_t>(arg);
		}

		template <typename arg_t>
		static auto as_char(const arg_t& arg) -> type::character<char_t>
		{
			if constexpr (::std::same_as<arg_t, type::character<char_t>>)
			{
				return arg;
			}
			else
			{
				return {};
			}
		}

		template <typename arg_t>
		static auto as_string(const arg_t& arg) -> const type::string<char_t>&
		{
			if constexpr (::std::same_as<arg_t, proxy<type::string<char_t>>>)
			{
				return *arg;
			}
			else
			{
				return null_string_val();
			}
		}

		template <typename arg_t>
		static auto as_list(const arg_t& arg) -> const type::list<char_t>&
		{
			if constexpr (::std::same_as<arg_t, proxy<type::list<char_t>>>)
			{
				return *arg;
			}
			else
			{
				return null_list_val();
			}
		}

		template <typename arg_t>
		static auto as_object(const arg_t& arg) -> const type::object<char_t>&
		{
			if constexpr (::std::same_as<arg_t, proxy<type::object<char_t>>>)
			{
				return *arg;
			}
			else
			{
				return null_object_val();
			}
		}

		static auto null_string_val() -> const type::string<char_t>&
		{
			static const auto null_val = type::string<char_t>{};
			return null_val;
		}
		static auto null_list_val() -> const type::list<char_t>&
		{
			static const auto null_val = type::list<char_t>{};
			return null_val;
		}
		static auto null_object_val() -> const type::object<char_t>&
		{
			static const auto null_val = type::object<char_t>{};
			return null_val;
		}
	};
}

namespace pdn::detail
{
	template <typename t>
	struct has_proxy
	{
		static constexpr bool value = false;
	};
	template <typename char_t>
	struct has_proxy<type::string<char_t>>
	{
		static constexpr bool value = true;
	};
	template <typename char_t>
	struct has_proxy<type::list<char_t>>
	{
		static constexpr bool value = true;
	};
	template <typename char_t>
	struct has_proxy<type::object<char_t>>
	{
		static constexpr bool value = true;
	};
	template <typename t>
	inline static constexpr bool has_proxy_v = has_proxy<t>::value;
}

namespace pdn::detail
{
	struct auto_int_tag_t {};
	struct i8_tag_t {};
	struct i16_tag_t {};
	struct i32_tag_t {};
	struct i64_tag_t {};
	struct auto_uint_tag_t {};
	struct u8_tag_t {};
	struct u16_tag_t {};
	struct u32_tag_t {};
	struct u64_tag_t {};
	struct f32_tag_t {};
	struct f64_tag_t {};
	struct bool_tag_t {};
	struct character_tag_t {};
	struct u8char_tag_t {};
	struct u16char_tag_t {};
	struct u32char_tag_t {};
	struct string_tag_t {};
	struct u8string_tag_t {};
	struct u16string_tag_t {};
	struct u32string_tag_t {};
	struct list_tag_t {};
	struct object_tag_t {};
}

namespace pdn::detail
{
	template <typename tag_t>
	concept sint_tag
		 = ::std::same_as<tag_t, auto_int_tag_t>
		|| ::std::same_as<tag_t, i8_tag_t>
		|| ::std::same_as<tag_t, i16_tag_t>
		|| ::std::same_as<tag_t, i32_tag_t>
		|| ::std::same_as<tag_t, i64_tag_t>;

	template <typename tag_t>
	concept sint_or_tag = sint_tag<tag_t> || type::concepts::pdn_sint<tag_t>;

	template <typename tag_t>
	concept uint_tag
		 = ::std::same_as<tag_t, auto_uint_tag_t>
		|| ::std::same_as<tag_t, u8_tag_t>
		|| ::std::same_as<tag_t, u16_tag_t>
		|| ::std::same_as<tag_t, u32_tag_t>
		|| ::std::same_as<tag_t, u64_tag_t>;

	template <typename tag_t>
	concept uint_or_tag = uint_tag<tag_t> || type::concepts::pdn_uint<tag_t>;

	template <typename tag_t>
	concept fp_tag
		 = ::std::same_as<tag_t, f32_tag_t>
		|| ::std::same_as<tag_t, f64_tag_t>;

	template <typename tag_t>
	concept fp_or_tag = fp_tag<tag_t> || type::concepts::pdn_fp<tag_t>;

	template <typename tag_t>
	concept char_series_tag
		 = ::std::same_as<tag_t, character_tag_t>
		|| ::std::same_as<tag_t, u8char_tag_t>
		|| ::std::same_as<tag_t, u16char_tag_t>
		|| ::std::same_as<tag_t, u32char_tag_t>;

	template <typename tag_t>
	concept string_series_tag
		 = ::std::same_as<tag_t, string_tag_t>
		|| ::std::same_as<tag_t, u8string_tag_t>
		|| ::std::same_as<tag_t, u16string_tag_t>
		|| ::std::same_as<tag_t, u32string_tag_t>;

	template <typename tag_t>
	concept type_tag
		 = sint_tag<tag_t>
		|| uint_tag<tag_t>
		|| fp_tag<tag_t>
		|| char_series_tag<tag_t>
		|| string_series_tag<tag_t>
		|| ::std::same_as<tag_t, bool_tag_t>
		|| ::std::same_as<tag_t, list_tag_t>
		|| ::std::same_as<tag_t, object_tag_t>;

	template <typename t>
	concept basic_type
		 = type::concepts::basic_type<t, unicode::u8char_t>
		|| ::std::same_as<t, type::u16char>
		|| ::std::same_as<t, type::u32char>;

	template <typename tar_t>
	concept as_tparam = basic_type<tar_t> || type_tag<tar_t>;

	template <typename tar_t, typename char_t>
	concept as_tparam_pure
		 = as_tparam<tar_t>
		|| ::std::same_as<tar_t, type::u8string>
		|| ::std::same_as<tar_t, type::u16string>
		|| ::std::same_as<tar_t, type::u32string>
		|| ::std::same_as<tar_t, type::list<char_t>>
		|| ::std::same_as<tar_t, type::object<char_t>>;;
}

namespace pdn::detail
{
	// use tag_to_type_t<tag_t, char_t> for the mapped type; other type default to itself.
	template <typename tag_t, typename char_t>
	struct tag_to_type                          { using type = tag_t; };
	template <typename char_t>
	struct tag_to_type<auto_int_tag_t,  char_t> { using type = type::auto_int; };
	template <typename char_t>
	struct tag_to_type<i8_tag_t,        char_t> { using type = type::i8; };
	template <typename char_t>
	struct tag_to_type<i16_tag_t,       char_t> { using type = type::i16; };
	template <typename char_t>
	struct tag_to_type<i32_tag_t,       char_t> { using type = type::i32; };
	template <typename char_t>
	struct tag_to_type<i64_tag_t,       char_t> { using type = type::i64; };
	template <typename char_t>
	struct tag_to_type<auto_uint_tag_t, char_t> { using type = type::auto_uint; };
	template <typename char_t>
	struct tag_to_type<u8_tag_t,        char_t> { using type = type::u8; };
	template <typename char_t>
	struct tag_to_type<u16_tag_t,       char_t> { using type = type::u16; };
	template <typename char_t>
	struct tag_to_type<u32_tag_t,       char_t> { using type = type::u32; };
	template <typename char_t>
	struct tag_to_type<u64_tag_t,       char_t> { using type = type::u64; };
	template <typename char_t>
	struct tag_to_type<f32_tag_t,       char_t> { using type = type::f32; };
	template <typename char_t>
	struct tag_to_type<f64_tag_t,       char_t> { using type = type::f64; };
	template <typename char_t>
	struct tag_to_type<bool_tag_t,      char_t> { using type = type::boolean; };
	template <typename char_t>
	struct tag_to_type<character_tag_t, char_t> { using type = type::character<char_t>; };
	template <typename char_t>
	struct tag_to_type<u8char_tag_t,    char_t> { using type = type::u8char; };
	template <typename char_t>
	struct tag_to_type<u16char_tag_t,   char_t> { using type = type::u16char; };
	template <typename char_t>
	struct tag_to_type<u32char_tag_t,   char_t> { using type = type::u32char; };
	template <typename char_t>
	struct tag_to_type<string_tag_t,    char_t> { using type = type::string<char_t>; };
	template <typename char_t>
	struct tag_to_type<u8string_tag_t,  char_t> { using type = type::u8string; };
	template <typename char_t>
	struct tag_to_type<u16string_tag_t, char_t> { using type = type::u16string; };
	template <typename char_t>
	struct tag_to_type<u32string_tag_t, char_t> { using type = type::u32string; };
	template <typename char_t>
	struct tag_to_type<list_tag_t,      char_t> { using type = type::list<char_t>; };
	template <typename char_t>
	struct tag_to_type<object_tag_t,    char_t> { using type = type::object<char_t>; };
	template <typename tag_t, typename char_t>
	using tag_to_type_t = typename tag_to_type<tag_t, char_t>::type;

	// return type helper for pdn::as series function
	template <typename target_t, typename char_t>
	struct as_rttype
	{
		static_assert(as_tparam_pure<target_t, char_t>, "[pdn] as_rttype requires that target_t is pdn::detail::concepts::as_tparam");
	};
	template <typename target_t, typename char_t> requires as_tparam_pure<target_t, char_t>
	struct as_rttype<target_t, char_t>
	{
		using type = tag_to_type_t<target_t, char_t>;
	};
	template <typename char_t>
	struct as_rttype<string_tag_t, char_t> // string -> const string&
	{
		using type = ::std::add_lvalue_reference_t<::std::add_const_t<tag_to_type_t<string_tag_t, char_t>>>;
	};
	template <typename char_t>
	struct as_rttype<u8string_tag_t, char_t> // string -> const string& | converted string -> string
	{
		using type = ::std::conditional_t<::std::is_same_v<unicode::u8char_t, char_t>,
			typename as_rttype<string_tag_t, char_t>::type,
			tag_to_type_t<u8string_tag_t, char_t>>;
	};
	template <typename char_t>
	struct as_rttype<u16string_tag_t, char_t> // string -> const string& | converted string -> string
	{
		using type = ::std::conditional_t<::std::is_same_v<unicode::u16char_t, char_t>,
			typename as_rttype<string_tag_t, char_t>::type,
			tag_to_type_t<u16string_tag_t, char_t>>;
	};
	template <typename char_t>
	struct as_rttype<u32string_tag_t, char_t> // string -> const string& | converted string -> string
	{
		using type = ::std::conditional_t<::std::is_same_v<unicode::u32char_t, char_t>,
			typename as_rttype<string_tag_t, char_t>::type,
			tag_to_type_t<u32string_tag_t, char_t>>;
	};
	template <typename char_t>
	struct as_rttype<type::u8string, char_t> // string -> const string& | converted string -> string
	{
		using type = typename as_rttype<u8string_tag_t, char_t>::type;
	};
	template <typename char_t>
	struct as_rttype<type::u16string, char_t> // string -> const string& | converted string -> string
	{
		using type = typename as_rttype<u16string_tag_t, char_t>::type;
	};
	template <typename char_t>
	struct as_rttype<type::u32string, char_t> // string -> const string& | converted string -> string
	{
		using type = typename as_rttype<u32string_tag_t, char_t>::type;
	};
	template <typename char_t>
	struct as_rttype<list_tag_t, char_t> // list -> const list&
	{
		using type = ::std::add_lvalue_reference_t<::std::add_const_t<tag_to_type_t<list_tag_t, char_t>>>;
	};
	template <typename char_t>
	struct as_rttype<type::list<char_t>, char_t> // list -> const list&
	{
		using type = typename as_rttype<list_tag_t, char_t>::type;
	};
	template <typename char_t>
	struct as_rttype<object_tag_t, char_t> // object -> const object&
	{
		using type = ::std::add_lvalue_reference_t<::std::add_const_t<tag_to_type_t<object_tag_t, char_t>>>;
	};
	template <typename char_t>
	struct as_rttype<type::object<char_t>, char_t> // list -> const list&
	{
		using type = typename as_rttype<object_tag_t, char_t>::type;
	};
	template <typename target_t, typename char_t>
	using as_rttype_t = typename as_rttype<target_t, char_t>::type;
}

#endif
