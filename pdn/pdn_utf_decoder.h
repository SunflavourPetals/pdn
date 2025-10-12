#ifndef PDN_Header_pdn_utf_decoder
#define PDN_Header_pdn_utf_decoder

#include <type_traits>
#include <utility>

#include "pdn_unicode_base.h"
#include "pdn_utf8_base.h"
#include "pdn_utf8_decoder.h"
#include "pdn_utf16_base.h"
#include "pdn_utf16_decoder.h"
#include "pdn_utf32_base.h"
#include "pdn_utf32_decoder.h"

namespace pdn::unicode::detail
{
	template <typename type>
	concept utf8_iterator = requires (type it)
	{
		requires concepts::utf8_code_unit<::std::remove_reference_t<decltype(*it)>>;
		++it;
	};
	template <typename type>
	concept utf16_iterator = requires (type it)
	{
		requires concepts::utf16_code_unit<::std::remove_reference_t<decltype(*it)>>;
		++it;
	};
	template <typename type>
	concept utf32_iterator = requires (type it)
	{
		requires concepts::utf32_code_unit<::std::remove_reference_t<decltype(*it)>>;
		++it;
	};
}

namespace pdn::unicode
{
	template <bool reach_next_code_point, detail::utf8_iterator it_begin_t, typename it_end_t>
	auto decode(it_begin_t&& begin, it_end_t end)
	{
		return utf8::decode<reach_next_code_point>(::std::forward<it_begin_t>(begin), ::std::move(end));
	}
	template <bool reach_next_code_point, detail::utf16_iterator it_begin_t, typename it_end_t>
	auto decode(it_begin_t&& begin, it_end_t end)
	{
		return utf16::decode<reach_next_code_point>(::std::forward<it_begin_t>(begin), ::std::move(end));
	}
	template <bool reach_next_code_point, detail::utf32_iterator it_begin_t, typename it_end_t>
	auto decode(it_begin_t&& begin, it_end_t end)
	{
		return utf32::decode<reach_next_code_point>(::std::forward<it_begin_t>(begin), ::std::move(end));
	}
}

namespace pdn::unicode::detail
{
	template <typename char_t> struct suitable_decoder {};
	template <> struct suitable_decoder<u8char_t>  { using type = utf8 ::decoder; };
	template <> struct suitable_decoder<u16char_t> { using type = utf16::decoder; };
	template <> struct suitable_decoder<u32char_t> { using type = utf32::decoder; };

	template <typename char_t>
	using suitable_decoder_t = suitable_decoder<char_t>::type;
}

namespace pdn::unicode
{
	template <concepts::code_unit char_t>
	using decoder = detail::suitable_decoder_t<char_t>;
}

#endif
