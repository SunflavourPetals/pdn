#ifndef PDN_Header_pdn_convert_decision
#define PDN_Header_pdn_convert_decision

#include <string>
#include <utility>
#include <concepts>
#include <functional>
#include <type_traits>

#include "pdn_unicode.h"

namespace pdn::unicode::concepts
{
	namespace detail
	{
		template <typename string_t, typename code_unit_t>
		concept convert_src_base = requires(string_t source)
		{
			typename string_t::value_type;
			typename string_t::iterator;
				requires ::std::same_as<typename string_t::value_type, code_unit_t>;
				requires ::std::random_access_iterator<typename string_t::iterator>;
		};
		template <typename string_t, typename code_unit_t>
		concept convert_des_base = requires(string_t destination, ::std::basic_string<code_unit_t> s)
		{
			typename string_t::value_type;
			requires ::std::same_as<typename string_t::value_type, code_unit_t>;
			destination.append(s.cbegin(), s.cend());
		};
	}

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
	template <concepts::convert_src source_string_view, concepts::convert_des target_string>
	struct convert_decision
	{
		using source_char        = source_string_view::value_type;
		using target_char        = target_string::value_type;
		using decoder_type       = decoder<source_char>;
		using encoder_type       = encoder<target_char>;
		using decode_result_type = decoder_type::result_type;
		using encode_result_type = encoder_type::result_type;

		template <bool reach_next_code_point = false>
		inline static auto decode(auto&& begin, auto end) -> decode_result_type
		{
			return decoder_type::template decode<reach_next_code_point>(begin, end);
		}
		inline static auto encode(code_point_t code_point) noexcept -> encode_result_type
		{
			return encoder_type::encode(code_point);
		}
	};
}

#endif
