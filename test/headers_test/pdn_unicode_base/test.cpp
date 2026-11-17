// Tests for pdn/include/pdn/pdn_unicode_base.h.
//
// This header is the bottom of the Unicode layer: character and string aliases for the three
// encodings, the code point predicates every decoder and encoder starts with, the BOM tables,
// the encode-type/endian mappings, the replacement character, and the _s/_sv literals.
// Almost everything here is constexpr, so most checks are static_asserts; main() only exercises
// the literals at run time.

#include <pdn/pdn_unicode_base.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace
{
	using namespace pdn::unicode;
	using namespace pdn::literals::unicode_literals;

	// ---- aliases ---------------------------------------------------------------
	static_assert(::std::is_same_v<code_point_t, char32_t>);
	static_assert(::std::is_same_v<u8char_t, char8_t>);
	static_assert(::std::is_same_v<u16char_t, char16_t>);
	static_assert(::std::is_same_v<u32char_t, char32_t>);

	static_assert(::std::is_same_v<u8string, ::std::u8string>);
	static_assert(::std::is_same_v<u16string, ::std::u16string>);
	static_assert(::std::is_same_v<u32string, ::std::u32string>);
	static_assert(::std::is_same_v<ucpstring, ::std::basic_string<char32_t>>);
	static_assert(::std::is_same_v<u8string_view, ::std::u8string_view>);
	static_assert(::std::is_same_v<u16string_view, ::std::u16string_view>);
	static_assert(::std::is_same_v<u32string_view, ::std::u32string_view>);
	static_assert(::std::is_same_v<ucpstring_view, ::std::basic_string_view<char32_t>>);

	// ---- code point predicates -------------------------------------------------
	static_assert(min_leading_surrogate == 0xD800u);
	static_assert(max_leading_surrogate == 0xDBFFu);
	static_assert(min_trailing_surrogate == 0xDC00u);
	static_assert(max_trailing_surrogate == 0xDFFFu);

	// A code point is anything below 0x110000, surrogates included; excluding them is the job of
	// is_scalar_value, which the decoders use before a value may enter a scalar-value token.
	static_assert(is_code_point(0x0000u));
	static_assert(is_code_point(0xD800u));
	static_assert(is_code_point(0x10FFFFu));
	static_assert(!is_code_point(0x110000u));

	static_assert(is_leading_surrogate(0xD800u));
	static_assert(is_leading_surrogate(0xDBFFu));
	static_assert(!is_leading_surrogate(0xD7FFu));
	static_assert(!is_leading_surrogate(0xDC00u));

	static_assert(is_trailing_surrogate(0xDC00u));
	static_assert(is_trailing_surrogate(0xDFFFu));
	static_assert(!is_trailing_surrogate(0xDBFFu));
	static_assert(!is_trailing_surrogate(0xE000u));

	static_assert(is_surrogate(0xD800u));
	static_assert(is_surrogate(0xDC00u));
	static_assert(is_surrogate(0xDFFFu));
	static_assert(!is_surrogate(0xD7FFu));
	static_assert(!is_surrogate(0xE000u));
	static_assert(is_non_surrogate(0x0041u));

	static_assert(is_scalar_value(0x0000u));
	static_assert(is_scalar_value(0x0041u));
	static_assert(is_scalar_value(0x10FFFFu));
	static_assert(!is_scalar_value(0xD800u));
	static_assert(!is_scalar_value(0xDFFFu));
	static_assert(!is_scalar_value(0x110000u));

	static_assert(is_in_BMP(0x0000u));
	static_assert(is_in_BMP(0xFFFFu));
	static_assert(!is_in_BMP(0x10000u));

	// ---- concepts --------------------------------------------------------------
	static_assert(concepts::utf8_code_unit<u8char_t>);
	static_assert(concepts::utf8_code_unit<const u8char_t>);
	static_assert(!concepts::utf8_code_unit<char>);
	static_assert(!concepts::utf8_code_unit<unsigned char>);
	static_assert(concepts::utf16_code_unit<u16char_t>);
	static_assert(!concepts::utf16_code_unit<u8char_t>);
	static_assert(concepts::utf32_code_unit<u32char_t>);

	static_assert(concepts::code_unit<u8char_t>);
	static_assert(concepts::code_unit<u16char_t>);
	static_assert(concepts::code_unit<u32char_t>);
	static_assert(!concepts::code_unit<char>);
	static_assert(!concepts::code_unit<wchar_t>);
	static_assert(!concepts::code_unit<void>);

	// ---- BOM tables -------------------------------------------------------------
	static_assert(bom::utf8.size() == 3);
	static_assert(bom::utf16_le.size() == 2);
	static_assert(bom::utf16_be.size() == 2);
	static_assert(bom::utf32_le.size() == 4);
	static_assert(bom::utf32_be.size() == 4);

	static_assert(bom::utf8 == ::std::array<bom::byte_t, 3>{ 0xEF, 0xBB, 0xBF });
	static_assert(bom::utf16_le == ::std::array<bom::byte_t, 2>{ 0xFF, 0xFE });
	static_assert(bom::utf16_be == ::std::array<bom::byte_t, 2>{ 0xFE, 0xFF });
	static_assert(bom::utf32_le == ::std::array<bom::byte_t, 4>{ 0xFF, 0xFE, 0x00, 0x00 });
	static_assert(bom::utf32_be == ::std::array<bom::byte_t, 4>{ 0x00, 0x00, 0xFE, 0xFF });

	// The UTF-32LE mark starts with the UTF-16LE mark, which is why the BOM reader tests the
	// longer marks first; the tables have to keep that prefix relationship.
	static_assert(::std::equal(bom::utf16_le.cbegin(), bom::utf16_le.cend(), bom::utf32_le.cbegin()));

	// ---- encode type / endian ----------------------------------------------------
	static_assert(to_encode_type(bom_type::no_bom) == encode_type::utf8);
	static_assert(to_encode_type(bom_type::utf8) == encode_type::utf8);
	static_assert(to_encode_type(bom_type::utf16_le) == encode_type::utf16_le);
	static_assert(to_encode_type(bom_type::utf16_be) == encode_type::utf16_be);
	static_assert(to_encode_type(bom_type::utf32_le) == encode_type::utf32_le);
	static_assert(to_encode_type(bom_type::utf32_be) == encode_type::utf32_be);

	static_assert(to_byte_size(bom_type::no_bom) == 0);
	static_assert(to_byte_size(bom_type::utf8) == bom::utf8.size());
	static_assert(to_byte_size(bom_type::utf16_le) == bom::utf16_le.size());
	static_assert(to_byte_size(bom_type::utf16_be) == bom::utf16_be.size());
	static_assert(to_byte_size(bom_type::utf32_le) == bom::utf32_le.size());
	static_assert(to_byte_size(bom_type::utf32_be) == bom::utf32_be.size());

	static_assert(to_endian(encode_type::utf8) == ::std::endian::native);
	static_assert(to_endian(encode_type::utf16_le) == ::std::endian::little);
	static_assert(to_endian(encode_type::utf16_be) == ::std::endian::big);
	static_assert(to_endian(encode_type::utf32_le) == ::std::endian::little);
	static_assert(to_endian(encode_type::utf32_be) == ::std::endian::big);

	// The variable templates and the functions are two parallel spellings of the same mapping;
	// they must not drift apart.
	static_assert(type_traits::encode_type_from_bom<bom_type::no_bom> == to_encode_type(bom_type::no_bom));
	static_assert(type_traits::encode_type_from_bom<bom_type::utf8> == to_encode_type(bom_type::utf8));
	static_assert(type_traits::encode_type_from_bom<bom_type::utf16_be> == to_encode_type(bom_type::utf16_be));
	static_assert(type_traits::encode_type_from_bom<bom_type::utf32_le> == to_encode_type(bom_type::utf32_le));
	static_assert(type_traits::endian_from_encode_type<encode_type::utf8> == to_endian(encode_type::utf8));
	static_assert(type_traits::endian_from_encode_type<encode_type::utf16_le> == to_endian(encode_type::utf16_le));
	static_assert(type_traits::endian_from_encode_type<encode_type::utf16_be> == to_endian(encode_type::utf16_be));
	static_assert(type_traits::endian_from_encode_type<encode_type::utf32_be> == to_endian(encode_type::utf32_be));

	static_assert(::std::is_same_v<type_traits::code_unit_t<encode_type::utf8>, u8char_t>);
	static_assert(::std::is_same_v<type_traits::code_unit_t<encode_type::utf16_le>, u16char_t>);
	static_assert(::std::is_same_v<type_traits::code_unit_t<encode_type::utf16_be>, u16char_t>);
	static_assert(::std::is_same_v<type_traits::code_unit_t<encode_type::utf32_le>, u32char_t>);
	static_assert(::std::is_same_v<type_traits::code_unit_t<encode_type::utf32_be>, u32char_t>);

	// ---- replacement character ---------------------------------------------------
	constexpr auto replacement8 = get_replace<u8char_t>();
	static_assert(replacement8.size() == 3);
	static_assert(replacement8[0] == u8char_t(0xEF));
	static_assert(replacement8[1] == u8char_t(0xBF));
	static_assert(replacement8[2] == u8char_t(0xBD));

	constexpr auto replacement16 = get_replace<u16char_t>();
	static_assert(replacement16.size() == 1);
	static_assert(replacement16[0] == char16_t(0xFFFD));

	constexpr auto replacement32 = get_replace<u32char_t>();
	static_assert(replacement32.size() == 1);
	static_assert(replacement32[0] == char32_t(0xFFFD));

	constexpr auto replacement8_cv = get_replace<const u8char_t>();
	static_assert(::std::is_same_v<::std::decay_t<decltype(replacement8_cv)>, u8string_view>);

	// ---- code units to bytes ------------------------------------------------------
	static_assert(utf16::to_le_bytes(0x1234u) == ::std::array<::std::uint8_t, 2>{ 0x34, 0x12 });
	static_assert(utf16::to_be_bytes(0x1234u) == ::std::array<::std::uint8_t, 2>{ 0x12, 0x34 });
	static_assert(utf32::to_le_bytes(0x01020304u) == ::std::array<::std::uint8_t, 4>{ 0x04, 0x03, 0x02, 0x01 });
	static_assert(utf32::to_be_bytes(0x01020304u) == ::std::array<::std::uint8_t, 4>{ 0x01, 0x02, 0x03, 0x04 });

	static_assert(::std::is_same_v<utf8::code_unit_t, u8char_t>);
	static_assert(::std::is_same_v<utf8::string, u8string>);
	static_assert(::std::is_same_v<utf16::string_view, u16string_view>);
	static_assert(::std::is_same_v<utf32::string, u32string>);

	// ---- literals ------------------------------------------------------------------
	static_assert(u8"abc"_sv.size() == 3);
	static_assert(u"abc"_sv.size() == 3);
	static_assert(U"abc"_sv.size() == 3);
	static_assert(u8"abc"_s.size() == 3);
	static_assert(U"\U0001F600"_s.size() == 1);
	static_assert(::std::is_same_v<decltype(u8"abc"_s), u8string>);
	static_assert(::std::is_same_v<decltype(u8"abc"_sv), u8string_view>);
	static_assert(::std::is_same_v<decltype(u"abc"_sv), u16string_view>);
	static_assert(::std::is_same_v<decltype(U"abc"_sv), u32string_view>);
}

int main()
{
	u8string text8 = u8"hello"_s;
	u16string text16 = u"hello"_s;
	u32string text32 = U"hello"_s;
	if (text8.size() != 5 || text16.size() != 5 || text32.size() != 5) return 1;

	if (u8string_view{ text8 } != u8"hello"_sv) return 2;
	if (u16string_view{ text16 } != u"hello"_sv) return 3;
	if (u32string_view{ text32 } != U"hello"_sv) return 4;

	// A code point string holds one element per code point even above the BMP.
	ucpstring points = U"\U0001F600\U0001F601"_s;
	if (points.size() != 2) return 5;
	if (points[0] != 0x1F600u || points[1] != 0x1F601u) return 6;

	// The replacement character must survive a round trip through a view.
	if (get_replace<u8char_t>() != u8"\uFFFD"_sv) return 7;

	return 0;
}
