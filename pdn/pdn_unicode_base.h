#ifndef PDN_Header_pdn_unicode_base
#define PDN_Header_pdn_unicode_base

#include <bit>
#include <string>
#include <type_traits>

namespace pdn::unicode
{
#define PDN_Macro_Temp_make_code_type_family(name, type) \
	using name##_t                   = type; \
	template <typename traits        = ::std::char_traits<name##_t>,\
	          typename alloc         = ::std::allocator<name##_t>> \
	using basic_##name##_string      = ::std::basic_string<name##_t, traits, alloc>; \
	template <typename traits        = ::std::char_traits<name##_t>> \
	using basic_##name##_string_view = ::std::basic_string_view<name##_t, traits>; \
	using name##_string              = basic_##name##_string<>; \
	using name##_string_view         = basic_##name##_string_view<>;

	PDN_Macro_Temp_make_code_type_family(code_point, char32_t)
	PDN_Macro_Temp_make_code_type_family(utf_8_code_unit, char8_t)
	PDN_Macro_Temp_make_code_type_family(utf_16_code_unit, char16_t)
	PDN_Macro_Temp_make_code_type_family(utf_32_code_unit, char32_t)

#undef PDN_Macro_Temp_make_code_type_family

	enum class encode_type
	{
		unknown = 0,
		utf_8,
		utf_16_le,
		utf_16_be,
		utf_32_le,
		utf_32_be,
	};

	enum class bom_type
	{
		no_bom = 0,
		utf_8,
		utf_16_le,
		utf_16_be,
		utf_32_le,
		utf_32_be,
	};

	inline constexpr auto min_leading_surrogate  = code_point_t(0xD800);
	inline constexpr auto max_leading_surrogate  = code_point_t(0xDBFF);
	inline constexpr auto min_trailing_surrogate = code_point_t(0xDC00);
	inline constexpr auto max_trailing_surrogate = code_point_t(0xDFFF);

	inline constexpr bool is_code_point(code_point_t c) noexcept
	{
		return c < code_point_t(0x110000);
	}

	inline constexpr bool is_leading_surrogate(code_point_t c) noexcept
	{
		return (c & code_point_t(0xFFFF'FC00)) == code_point_t(0xD800); // ignore the low 10 bits
	}

	inline constexpr bool is_trailing_surrogate(code_point_t c) noexcept
	{
		return (c & code_point_t(0xFFFF'FC00)) == code_point_t(0xDC00); // ignore the low 10 bits
	}

	inline constexpr bool is_surrogate(code_point_t c) noexcept
	{
		return (c & code_point_t(0xFFFF'F800)) == code_point_t(0xD800); // ignore the low 11 bits
	}

	inline constexpr bool is_non_surrogate(code_point_t c) noexcept(noexcept(is_surrogate({})))
	{
		return !is_surrogate(c);
	}

	inline constexpr bool is_scalar_value(code_point_t c) noexcept(noexcept(is_non_surrogate({})) && noexcept(is_code_point({})))
	{
		return is_code_point(c) && is_non_surrogate(c);
	}

	inline constexpr bool in_BMP(code_point_t c) noexcept
	{
		return c < code_point_t(0x10000);
	}
}

namespace pdn::unicode::utility
{
	inline constexpr encode_type to_encode_type(bom_type bom_type) noexcept
	{
		switch (bom_type)
		{
		case bom_type::no_bom:
		case bom_type::utf_8:
			return encode_type::utf_8;
			break;
		case bom_type::utf_16_le:
			return encode_type::utf_16_le;
			break;
		case bom_type::utf_16_be:
			return encode_type::utf_16_be;
			break;
		case bom_type::utf_32_le:
			return encode_type::utf_32_le;
			break;
		case bom_type::utf_32_be:
			return encode_type::utf_32_be;
			break;
		default:
			break;
		}
		return encode_type::unknown;
	}

	inline constexpr ::std::endian to_endian(encode_type encode_type) noexcept
	{
		switch (encode_type)
		{
		case encode_type::unknown:
		case encode_type::utf_8:
			break;
		case encode_type::utf_16_le:
			return ::std::endian::little;
			break;
		case encode_type::utf_16_be:
			return ::std::endian::big;
			break;
		case encode_type::utf_32_le:
			return ::std::endian::little;
			break;
		case encode_type::utf_32_be:
			return ::std::endian::big;
			break;
		default:
			break;
		}
		return ::std::endian::native;
	}
}

namespace pdn::unicode::concepts
{
	template <typename char_t>
	concept unicode_code_unit =
		   ::std::is_same_v<::std::remove_cv_t<char_t>, utf_8_code_unit_t>
		|| ::std::is_same_v<::std::remove_cv_t<char_t>, utf_16_code_unit_t>
		|| ::std::is_same_v<::std::remove_cv_t<char_t>, utf_32_code_unit_t>;
}

namespace pdn::unicode::type_traits
{
	template <typename char_t>
	inline constexpr bool is_unicode_code_unit_v = concepts::unicode_code_unit<char_t>;

	template <typename char_t>
	struct is_unicode_code_unit : public ::std::bool_constant<is_unicode_code_unit_v<char_t>> {};

	template <bom_type>
	inline constexpr encode_type encode_type_from_bom = encode_type::unknown;
	template <>
	inline constexpr encode_type encode_type_from_bom<bom_type::no_bom> = encode_type::utf_8;
	template <>
	inline constexpr encode_type encode_type_from_bom<bom_type::utf_8> = encode_type::utf_8;
	template <>
	inline constexpr encode_type encode_type_from_bom<bom_type::utf_16_le> = encode_type::utf_16_le;
	template <>
	inline constexpr encode_type encode_type_from_bom<bom_type::utf_16_be> = encode_type::utf_16_be;
	template <>
	inline constexpr encode_type encode_type_from_bom<bom_type::utf_32_le> = encode_type::utf_32_le;
	template <>
	inline constexpr encode_type encode_type_from_bom<bom_type::utf_32_be> = encode_type::utf_32_be;

	template <encode_type>
	inline constexpr ::std::endian endian_from_encode_type = ::std::endian::native;
	template <>
	inline constexpr ::std::endian endian_from_encode_type<encode_type::utf_16_le> = ::std::endian::little;
	template <>
	inline constexpr ::std::endian endian_from_encode_type<encode_type::utf_16_be> = ::std::endian::big;
	template <>
	inline constexpr ::std::endian endian_from_encode_type<encode_type::utf_32_le> = ::std::endian::little;
	template <>
	inline constexpr ::std::endian endian_from_encode_type<encode_type::utf_32_be> = ::std::endian::big;

	template <encode_type e>
	struct code_unit { static_assert(e == encode_type::utf_8, "unknown encode type"); };
	template <>
	struct code_unit<encode_type::utf_8> { using type = char8_t; };
	template <>
	struct code_unit<encode_type::utf_16_le> { using type = char16_t; };
	template <>
	struct code_unit<encode_type::utf_16_be> { using type = char16_t; };
	template <>
	struct code_unit<encode_type::utf_32_le> { using type = char32_t; };
	template <>
	struct code_unit<encode_type::utf_32_be> { using type = char32_t; };
	template <encode_type e>
	using code_unit_t = typename code_unit<e>::type;
}

namespace pdn::unicode::dev_util
{
	template <typename char_t>
	concept code_unit_type_without_cv =
		    type_traits::is_unicode_code_unit_v<char_t>
		&& !::std::is_const_v<char_t>
		&& !::std::is_volatile_v<char_t>;
}

namespace pdn
{
	inline namespace literals
	{
		inline namespace unicode_literals
		{
#define make_unicode_user_defined_literals(name, suffix_s, suffix_sv) \
			[[nodiscard]] constexpr unicode::name##_string operator"" suffix_s(const unicode::name##_t* ptr, ::std::size_t length) \
			{ \
				return unicode::name##_string{ ptr, length }; \
			} \
			[[nodiscard]] constexpr unicode::name##_string_view operator"" suffix_sv(const unicode::name##_t* ptr, ::std::size_t length) \
			{ \
				return unicode::name##_string_view{ ptr, length }; \
			}

			make_unicode_user_defined_literals(code_point,       _us,   _usv)
			make_unicode_user_defined_literals(utf_8_code_unit,  _ucus, _ucusv)
			make_unicode_user_defined_literals(utf_16_code_unit, _ucus, _ucusv)
			make_unicode_user_defined_literals(utf_32_code_unit, _ucus, _ucusv)

#undef make_unicode_user_defined_literals
		}
	}
}

#endif
