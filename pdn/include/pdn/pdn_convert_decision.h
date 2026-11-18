#ifndef PDN_Header_pdn_convert_decision
#define PDN_Header_pdn_convert_decision

#include <string>
#include <iterator>
#include <concepts>

#include "pdn_unicode.h"

namespace pdn::unicode::concepts
{
	namespace detail
	{
		// A source needs a code-unit value_type plus const iterators that support subtraction:
		// code_convert walks a const source and reports error offsets as curr - cbegin(source).
		// Checking cbegin/cend instead of the nested iterator typedef is the accurate requirement
		// and slightly tighter: a type whose const_iterator is weaker than its mutable one is
		// rejected, while convert_decision itself only ever looks at value_type.
		template <typename string_t, typename code_unit_t>
		concept convert_src_base = requires(const string_t source)
		{
			typename string_t::value_type;
			requires ::std::same_as<typename string_t::value_type, code_unit_t>;
			requires ::std::random_access_iterator<decltype(::std::cbegin(source))>;
		};
		template <typename string_t, typename code_unit_t>
		concept convert_des_base = requires(string_t destination, ::std::basic_string<code_unit_t> s)
		{
			typename string_t::value_type;
			requires ::std::same_as<typename string_t::value_type, code_unit_t>;
			destination.insert(destination.end(), s.cbegin(), s.cend());
		};
	}

	// A source is a string-like sequence of one code unit type (string, string_view, vector,
	// array, deque); a destination is anything with insert(end(), first, last) over that same code
	// type. Only the code unit types matter to convert_decision below, not the containers
	// themselves.
	template <typename string_t> concept utf8_convert_src  = detail::convert_src_base<string_t, u8char_t>;
	template <typename string_t> concept utf16_convert_src = detail::convert_src_base<string_t, u16char_t>;
	template <typename string_t> concept utf32_convert_src = detail::convert_src_base<string_t, u32char_t>;
	template <typename string_t> concept utf8_convert_des  = detail::convert_des_base<string_t, u8char_t>;
	template <typename string_t> concept utf16_convert_des = detail::convert_des_base<string_t, u16char_t>;
	template <typename string_t> concept utf32_convert_des = detail::convert_des_base<string_t, u32char_t>;

	template <typename string_t>
	concept convert_src = utf8_convert_src<string_t> || utf16_convert_src<string_t> || utf32_convert_src<string_t>;
	template <typename string_t>
	concept convert_des = utf8_convert_des<string_t> || utf16_convert_des<string_t> || utf32_convert_des<string_t>;
}

namespace pdn::unicode
{
	// Picks the decoder for the source code unit type and the encoder for the target one; the two
	// string types are only used to name those code units. decode() forwards reach_next_code_point
	// to the decoder (see pdn_utf_decoder.h: false leaves the iterator on the last code unit),
	// encode() is noexcept because the encoders report failure through the result value.
	template <concepts::convert_src source_string_view, concepts::convert_des target_string>
	struct convert_decision
	{
		using source_char        = typename source_string_view::value_type;
		using target_char        = typename target_string::value_type;
		using decoder_type       = decoder<source_char>;
		using encoder_type       = encoder<target_char>;
		using decode_result_type = typename decoder_type::result_type;
		using encode_result_type = typename encoder_type::result_type;

		template <bool reach_next_code_point = false>
		static auto decode(auto&& begin, auto end) -> decode_result_type
		{
			return decoder_type::template decode<reach_next_code_point>(begin, end);
		}
		static auto encode(code_point_t code_point) noexcept -> encode_result_type
		{
			return encoder_type::encode(code_point);
		}
	};
}

#endif
